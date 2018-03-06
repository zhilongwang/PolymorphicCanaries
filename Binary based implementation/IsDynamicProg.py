import os  
import subprocess
global dynamicnum
dynamicnum = 0
global staticnum
staticnum=0

def run_cmd(cmd):  
    try:  
        import subprocess  
    except ImportError:  
        _, result_f, error_f = os.popen3(cmd)  
    else:  
        process = subprocess.Popen(cmd, shell = True,  
        stdout = subprocess.PIPE, stderr = subprocess.PIPE)  
        result_f, error_f = process.stdout, process.stderr  
  
    errors = error_f.read()  
    if errors:  pass  
    result_str = result_f.read().strip()  
    if result_f :   result_f.close()  
    if error_f  :    error_f.close()  
    print result_str 
    if result_str.find("not a dynamic executable")==-1:
        return 1
    else :
        if result_str.find("lib")==-1:
            return 0
        else :
            return -1;

def run_cmd_iself(cmd):  
    try:  
        import subprocess  
    except ImportError:  
        _, result_f, error_f = os.popen3(cmd)  
    else:  
        process = subprocess.Popen(cmd, shell = True,  
        stdout = subprocess.PIPE, stderr = subprocess.PIPE)  
        result_f, error_f = process.stdout, process.stderr  
  
    errors = error_f.read()  
    if errors:  pass  
    result_str = result_f.read().strip()  
    if result_f :   result_f.close()  
    if error_f  :    error_f.close()  
    print result_str 
    if result_str.find("no")==-1:
        #print "yes"
        return 1
    else :
        #print "no"
        return 0
    return -1



def dirlist(path, allfile): 
    global dynamicnum

    global staticnum 
    filelist =  os.listdir(path)  
  
    for filename in filelist:  
        filepath = os.path.join(path, filename)  
        if os.path.isdir(filepath):  
            dirlist(filepath, allfile)  
        else:  
            allfile.append(filepath)
            #print filepath
            #os.system("/home/zhujun/Desktop/ELFkickers/infect/infect"+)
            print filepath
            iself=run_cmd_iself("infect "+filepath)
            if iself==1:    
                print "iself"
                ret=run_cmd("ldd "+filepath)
                if ret==1:
                    dynamicnum=dynamicnum+1
                else :
                    if ret==0 :
                        staticnum=staticnum+1

    return allfile  

dirlist("/", [])  
print "dynamic_num:"
print dynamicnum
print "staticnum_num:"
print staticnum
