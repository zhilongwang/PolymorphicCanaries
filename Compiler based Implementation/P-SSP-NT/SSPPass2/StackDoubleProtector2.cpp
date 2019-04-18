#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Analysis/BranchProbabilityInfo.h"
#include "llvm/Analysis/EHPersonalities.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/IR/Attributes.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/DebugLoc.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/MDBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/User.h"
#include "llvm/Pass.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Target/TargetLowering.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/Target/TargetSubtargetInfo.h"
#include <utility>
#include "StackDoubleProtector2.h"

using namespace llvm;

#define DEBUG_TYPE "stack-double-canary"

STATISTIC(NumFunProtected, "Number of functions protected");
STATISTIC(NumAddrTaken, "Number of local variables that have their address taken.");

static cl::opt<bool> EnableSelectionDAGSP("enable-selectiondag-sdp2", cl::init(true), cl::Hidden);

char StackDoubleProtector2::ID = 0;
/*
INITIALIZE_PASS_BEGIN(StackDoubleProtector, DEBUG_TYPE, "Insert stack double protectors", false, true);
INITIALIZE_PASS_DEPENDENCY(TargetPassConfig);
INITIALIZE_PASS_END(StackDoubleProtector, DEBUG_TYPE, "Insert stack double protectors", false, true);*/

//FunctionPass *llvm::createStackDoubleProtectorPass() { return new StackDoubleProtector(); }

/*void StackDoubleProtector::adjustForColoring(const AllocaInst *From, const AllocaInst *To) {

//When coloring replaces one alloca with another, transfer the SSPLayoutKind tag from the remapped to the
//target alloca. The remapped alloca should have a size smaller than or equal to the replacement alloca.
SSPDLayoutMap::iterator I = layout.find(From);
if (I != layout.end()) {
SSPLayoutKind Kind = I->second;
layout.erase(I);

//Transfer the tag, but make sure that SSPLK_AddrOf does not overwrite
//SSPLK_SmallArray or SSPLK_LargeArray, and make sure that SSPLK_SmallArray
//does not overwrite SSPLK_LargeArray.
I = layout.find(To);
if (I == layout.end()) {
layout.insert(std::make_pair(To, Kind));
}
else if (I->second != SSPLK_LargeArray && Kind != SSPLK_AddrOf) {
I->second = Kind;
}
}
}*/

void StackDoubleProtector2::getAnalysisUsage(AnalysisUsage &AU) const {
	AU.addRequired<TargetPassConfig>();
	AU.addPreserved<DominatorTreeWrapperPass>();
}

/// \param [out] IsLarge is set to true if a protectable array is found and 
/// it is "large" (>= ssp-buffer-size). In the case of a structure with multiple
/// arrays, this gets set if any of them is large.
bool StackDoubleProtector2::ContainsProtectableArray(Type *Ty, bool &IsLarge, bool Strong, bool InStruct) const {
	if (!Ty)
		return false;
	if (ArrayType *AT = dyn_cast<ArrayType>(Ty)) {
		if (!AT->getElementType()->isIntegerTy(8)) {
			//If we're on a non-Darwin platform or we're inside of a structure, don't
			// add stack protectors unless the array is a character array.
			// However, in strong mode any array, regardless of type and size,
			// triggers a protector.
			if (!Strong && (InStruct || !Trip.isOSDarwin()))
				return false;
		}
		// If an array has more than SSPBufferSize bytes of allocated space, then
		// we emit stack double protectors.
		if (SSPBufferSize <= M->getDataLayout().getTypeAllocSize(AT)) {
			IsLarge = true;
			return true;
		}
		if (Strong) {
			//Require a double protector for all arrays in strong mode.
			return true;
		}
	}
	const StructType *ST = dyn_cast<StructType>(Ty);
	if (!ST)
		return false;
	bool NeedsProtector = false;
	for (StructType::element_iterator I = ST->element_begin(), E = ST->element_end(); I != E; ++I) {
		if (ContainsProtectableArray(*I, IsLarge, Strong, true)) {
			// If the element is a protectable array and is large(>= SSPBufferSize)
			// then we are done. If the protectable array is not large, then
			// keep looking in case a subsequent element is a large array.
			if (IsLarge)
				return true;
			NeedsProtector = true;
		}
	}
	return NeedsProtector;
}

bool StackDoubleProtector2::HasAddressTaken(const Instruction *AI) {
	for (const User *U : AI->users()) {
		if (const StoreInst *SI = dyn_cast<StoreInst>(U)) {
			if (AI == SI->getValueOperand())
				return true;
		}
		else if (const PtrToIntInst *SI = dyn_cast<PtrToIntInst>(U)) {
			if (AI == SI->getOperand(0))
				return true;
		}
		else if (isa<CallInst>(U)) {
			return true;
		}
		else if (isa<InvokeInst>(U)) {
			return true;
		}
		else if (const SelectInst *SI = dyn_cast<SelectInst>(U)) {
			if (HasAddressTaken(SI))
				return true;
		}
		else if (const PHINode *PN = dyn_cast<PHINode>(U)) {
			// Keep track of what PHI nodes we have already visited to ensure
			// they are only visited once.
			if (VisitedPHIs.insert(PN).second)
				if (HasAddressTaken(PN))
					return true;
		}
		else if (const GetElementPtrInst *GEP = dyn_cast<GetElementPtrInst>(U)) {
			if (HasAddressTaken(GEP))
				return true;
		}
		else if (const BitCastInst *BI = dyn_cast<BitCastInst>(U)) {
			if (HasAddressTaken(BI))
				return true;
		}
	}
	return false;
}

/// \brief Check whether or not this function needs a stack protector based
/// upon the stack protector level.
///
/// We use two heuristics: a standard (ssp) and strong (sspstrong).
/// The standard heuristic which will add a guard variable to functions that
/// call alloca with a either a variable size or a size >= SSPBufferSize,
/// functions with character buffers larger than SSPBufferSize, and functions
/// with aggregates containing character buffers larger than SSPBufferSize. The
/// strong heuristic will add a guard variables to functions that call alloca
/// regardless of size, functions with any buffer regardless of type and size,
/// functions with aggregates that contain any buffer regardless of type and
/// size, and functions that contain stack-based variables that have had their
/// address taken.
bool StackDoubleProtector2::RequiresStackProtector() {
	bool Strong = false;
	bool NeedsProtector = false;
	Strong = true;
	for (const BasicBlock &BB : *F) {
		for (const Instruction &I : BB) {
			if (const AllocaInst *AI = dyn_cast<AllocaInst>(&I)) {

				if (AI->isArrayAllocation()) {
					NeedsProtector = true;
					continue;
				}
				if (HasAddressTaken(AI)) {
					++NumAddrTaken;
					NeedsProtector = true;
				}

			}
		}
	}

	return NeedsProtector;
}

static Constant* SegmentOffsetStack(IRBuilder<> &IRB, unsigned Offset, unsigned AddressSpace) {
	return ConstantExpr::getIntToPtr(
		ConstantInt::get(Type::getInt32Ty(IRB.getContext()), Offset),
		Type::getInt8PtrTy(IRB.getContext())->getPointerTo(AddressSpace)
	);
}
//Get the stack guard value 
static Value* getTheStackGuardValue(IRBuilder<> &IRB, unsigned offset) {
	return SegmentOffsetStack(IRB, offset, 257); //257 is the x86_64's fs segment
}
static bool CreatePrologue(Function *F, Module *M, ReturnInst *RI,
	const TargetLoweringBase *TLI, AllocaInst *&AI1, AllocaInst *&AI2) {
	bool SupportSelectionDAGSP = false;
	IRBuilder<> B(&F->getEntryBlock().front());
	PointerType *PtrTy = Type::getInt8PtrTy(RI->getContext());

	AI1 = B.CreateAlloca(PtrTy, nullptr, "StackDoubleStackSlot1");
	AI2 = B.CreateAlloca(PtrTy, nullptr, "RandomValue");
	
	std::vector<Type *> arg_type;
	//arg_type.push_back(Type::getInt64Ty(F->getContext()));
	//arg_type.push_back(Type::getInt32Ty(F->getContext()));
	//auto* helperTy = FunctionType::get(Type::getVectorElementType);
	Function *fun = Intrinsic::getDeclaration(F->getParent(), Intrinsic::x86_rdrand_64, arg_type);
	CallInst* result = B.CreateCall(fun, {});
	Value* randomValue = nullptr;
	Value* randomValuePtr = nullptr;
	//B.CreateRet(result);
	//result->dump();
	if (dyn_cast<StructType>(result->getType())) {
		//errs() << "Hello\n";
		randomValue = B.CreateExtractValue(result, (uint64_t)0);
		randomValuePtr = B.CreateIntToPtr(randomValue, Type::getInt8PtrTy(B.getContext()));
		B.CreateStore(randomValuePtr, AI2, true);
	}

	//Value* DoubleGuard1 = getTheStackGuardValue(B, 0x2a8);
	//Value* DoubleGuard2 = getTheStackGuardValue(B, 0x2b0);
	//mov %fs:0x2a8, %reg
	//Value* loadedDoubleGuard1 = B.CreateLoad(DoubleGuard1, true, "stackDoubleGuard1");
	//mov %fs:0x2b0, %reg
	//Value* loadedDoubleGuard2 = B.CreateLoad(DoubleGuard2, true, "stackDoubleGuard2");

	Value* stackGuard = getTheStackGuardValue(B, 0x28);
	LoadInst* loadStackGuard = B.CreateLoad(stackGuard, true);
	auto DL = M->getDataLayout();
	Value* stackI = B.CreatePtrToInt(loadStackGuard, DL.getIntPtrType(stackGuard->getType()));
	Value* randomI = B.CreatePtrToInt(randomValuePtr, DL.getIntPtrType(randomValuePtr->getType()));

	Value* another_stackGuard = B.CreateIntToPtr(B.CreateXor(stackI, randomI), loadStackGuard->getType());
	B.CreateStore(another_stackGuard, AI1, true);
	//B.CreateStore(loadedDoubleGuard2, AI2, true);
	return SupportSelectionDAGSP;
}



//CreateFailBB - Create a basic block to jump to when the stack double protector check fails.
BasicBlock *StackDoubleProtector2::CreateFailBB() {
	LLVMContext &Context = F->getContext();
	BasicBlock *FailBB = BasicBlock::Create(Context, "CallStackCheckFailBlk", F);
	IRBuilder<> B(FailBB);
	B.SetCurrentDebugLocation(DebugLoc::get(0, 0, F->getSubprogram()));
	Constant *StackChkFail = M->getOrInsertFunction("__stack_chk_fail", Type::getVoidTy(Context));

	B.CreateCall(StackChkFail, {});

	B.CreateUnreachable();
	return FailBB;
}
bool StackDoubleProtector2::InsertStackDoubleProtectors() {
	AllocaInst *AI1 = nullptr;    //Place on stack that stores the stack guard.

	AllocaInst *AI2 = nullptr;

	const DataLayout &DL = M->getDataLayout();

	for (Function::iterator I = F->begin(), E = F->end(); I != E;) {

		BasicBlock *BB = &*I++;

		ReturnInst *RI = dyn_cast<ReturnInst>(BB->getTerminator());

		if (!RI)

			continue;



		if (!HasPrologue) {

			HasPrologue = true;

			CreatePrologue(F, M, RI, TLI, AI1, AI2);

		}



		//SelectionDAG based code generation. Nothing else needs to be done here.

		// The epilogue instruemntation is postponed to SelectionDAG

		//if (SupportsSelectionDAGSP)

		//break;





		//Create epilogue instrumentation.

		//Generate the epilogue with inline instrumentation.

		//For each block with a return instruction, convert this:

		// return:

		//    ...

		//	  ret ...

		// into this:

		//	

		//	return:

		//	   ...

		//		%1 = load StackDoubleGuard1

		//		%2 = load StackDoubleGuard2

		//		%3 = xor %1, %2

		//		%4 = <stack guard>

		//		%5 = cmp i1 %3, %4

		//		br i1 %3, label %SP_return, label %CallStackCheckFailBlk

		//

		//		SP_return:

		//		ret...

		//		

		//		CallStackCheckFailBlk:

		//			call void &__stack_chk_fail()

		//			unreachable



		//Create the FailBB. We duplicate the BB every time since the MI tail merge

		// pass will merge together all of the various BB into one including

		// fail BB gererated by the stack protector pseudo instruction.

		BasicBlock *FailBB = CreateFailBB();



		//Split the basic block before the return instruction.

		BasicBlock *NewBB = BB->splitBasicBlock(RI->getIterator(), "SP_return");



		// Update the dominator tree if we need to

		if (DT && DT->isReachableFromEntry(BB)) {

			DT->addNewBlock(NewBB, BB);

			DT->addNewBlock(FailBB, BB);

		}



		//Remove default branch instruction to the new BB.

		BB->getTerminator()->eraseFromParent();



		//Move the newly created vasic block to the point right after the old

		// basic block so that it's in the "fall through" position.

		NewBB->moveAfter(BB);



		//Generate the stack double protector instructions in the old basic block.

		IRBuilder<> B(BB);

		Value* stackGuard = getTheStackGuardValue(B, 0x28);

		LoadInst *LI1 = B.CreateLoad(AI1, true);

		LoadInst *LI2 = B.CreateLoad(AI2, true);



		Value* LII = B.CreatePtrToInt(LI1, DL.getIntPtrType(LI1->getType()));

		Value* LII2 = B.CreatePtrToInt(LI2, DL.getIntPtrType(LI2->getType()));



		//Value* LII = B.CreateBitCast(LI1, Type::getInt64Ty(B.getContext()));

		//Value* LII2 = B.CreateBitCast(LI2, Type::getInt64Ty(B.getContext()));

		Value* loadedStackGuard = B.CreateLoad(stackGuard, true, "stackdoubleGuard");

		Value* stackGuard_stack = B.CreateIntToPtr(B.CreateXor(LII, LII2), loadedStackGuard->getType());



		Value* Cmp = B.CreateICmpEQ(loadedStackGuard, stackGuard_stack);



		//branch optimization

		auto SuccessProb = BranchProbabilityInfo::getBranchProbStackProtector(true);

		auto FailureProb = BranchProbabilityInfo::getBranchProbStackProtector(false);



		MDNode *weights = MDBuilder(F->getContext()).createBranchWeights(SuccessProb.getNumerator(),

			FailureProb.getNumerator());

		B.CreateCondBr(Cmp, NewBB, FailBB, weights);



	}
}

bool StackDoubleProtector2::runOnFunction(Function &Fn) {
	F = &Fn;
	M = F->getParent();
	DominatorTreeWrapperPass *DTWP = getAnalysisIfAvailable<DominatorTreeWrapperPass>();

	DT = DTWP ? &DTWP->getDomTree() : nullptr;

	TM = &getAnalysis<TargetPassConfig>().getTM<TargetMachine>();
	Trip = TM->getTargetTriple();
	TLI = TM->getSubtargetImpl(Fn)->getTargetLowering();
	HasPrologue = false;
	HasIRCheck = false;

	Attribute Attr = Fn.getFnAttribute("stack-protector-buffer-size");
	if (Attr.isStringAttribute() &&
		Attr.getValueAsString().getAsInteger(10, SSPBufferSize))
		return false;

	if (!RequiresStackProtector())
		return false;

	++NumFunProtected;
	return InsertStackDoubleProtectors();
}

static RegisterPass<StackDoubleProtector2> X("SSPPass2", "Stack Double Protector2", false, false);


