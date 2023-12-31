import subprocess
import base64
def main():
    newValueText = ""
    final_content = []
    with open("delay.txt", 'r') as file:
        final_content = file.readlines()
    with open("delay.txt", 'w+') as file:
        print(final_content)
        for item in final_content:
            valueCmd = removespace(item).split(",")
            if(len(valueCmd)>2):
                print(valueCmd[2])
                if (valueCmd[2]=="await"):
                    reurnvalue, _ = run_linux_command(base64_decode(valueCmd[1]))
                    encoded_bytes = base64.b64encode(reurnvalue.encode('utf-8'))
                    valueCmd[2] = "success"
                    valueCmd[3] = str(encoded_bytes.decode('utf-8'))
                    newValueText += f"{valueCmd[0]}, {valueCmd[1]}, {valueCmd[2]}, {valueCmd[3]}\n"
                    print(newValueText)
                else:
                    newValueText += f"{valueCmd[0]}, {valueCmd[1]}, {valueCmd[2]}, {valueCmd[3]}\n"
        file.write(newValueText)
    
def removespace(strTemp):
    return strTemp.replace(" ", "")
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
main()