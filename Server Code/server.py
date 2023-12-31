import os
import sys
import json
import subprocess
import math
import base64
import testLeetcode
# disclaimer, this code is heavily edited, so if you want to run this code you must adjust your code to your needed
sys.path.insert(0, os.path.dirname(__file__))

def application(environ, start_response):
    isRedirect = False
    redirectSession = ""
    response = ""
    
    getPath = str(environ["PATH_INFO"])
    getPath = getPath[1:len(getPath)].split('/')
    if len(getPath) >= 1:
        if getPath[0] == "delayrequestForPass":
            with open('delay.txt', 'a+') as file:
                file.seek(0)
                lines = file.readlines()
                number_of_lines = len(lines)
                idProc = str(number_of_lines)+getPath[1]
                file.write(f"{idProc}, {getPath[1]} , await, none\n")
            response=idProc
        if getPath[0] == "getDelay":
            try:
                with open('delay.txt', 'a+') as file:
                    file.seek(0)
                    final_content = file.readlines()
                    for item in final_content:
                        valueCmd = removespace(item).split(",")
                        if(len(valueCmd)>3):
                            if(str(valueCmd[0])==str(getPath[1])):
                                response=base64_decode(valueCmd[3])
            except:
                response="failed"
        if getPath[0] == "requestForPass":
            try:
                responseTemp,_ = run_linux_command(base64_decode(getPath[1]))
                response+=responseTemp
            except KeyError:
                response="Failed"
        if getPath[0] == "savefile":
            filename = base64_decode(getPath[1])
            value = base64_decode(getPath[2])
            edit_or_create_text_file(filename, value)
            response="success"
        if getPath[0] == "leetcode":
            # endpoint to play leetcode in endpoint
            arg1 = "difficulty"
            arg2 = ""
            arg3 = ""
            arg4 = ""
            if len(getPath)>1:
                arg1 = getPath[1]
            if len(getPath)>2:
                arg2 = getPath[2]
            if len(getPath)>3:
                arg3 = getPath[3]
            if len(getPath)>4:
                arg4 = getPath[4]
            response+=testLeetcode.handleRequesy(["leetcode", arg1, arg2, arg3, arg4])
    start_response('200 OK', [('Content-Type', 'text/plain')])
    return [response.encode()]
def getItemCheckNull(dictVal, indexVal):
    try:
        return str(dictVal[indexVal])
    except KeyError:
        return ""
def base64_decode(encoded_string):
    decoded_bytes = base64.b64decode(encoded_string.encode('utf-8'))
    decoded_string = decoded_bytes.decode('utf-8')
    return decoded_string

def run_linux_command(command):
    try:
        result = subprocess.run(command, shell=True, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)

        output = result.stdout.strip()
        error = result.stderr.strip()

        return output, error
    except subprocess.CalledProcessError as e:
        error_message = e.stderr.strip() if e.stderr else e.stdout
        return "failed", error_message
def edit_or_create_text_file(file_path, new_content=None):
    try:
        with open(file_path, 'r') as file:
            content = file.read()
        if new_content is not None:
            content = new_content
        with open(file_path, 'w') as file:
            file.write(content)
        
        print(f"File '{file_path}' edited successfully.")
    except FileNotFoundError:
        # If the file doesn't exist, create a new file
        with open(file_path, 'w') as file:
            if new_content is not None:
                file.write(new_content)
        print(f"File '{file_path}' created successfully.")
def removespace(strTemp):
    return strTemp.replace(" ", "")