#include <imgui_vita.h>
#include <stdio.h>
#include <vitaGL.h>
#include <string>
#include <vector>
#include <math.h>
#include <sstream>
#include <cstdlib>
#include <psp2/sysmodule.h>
#include <psp2/kernel/threadmgr.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/display.h>
#include <psp2/ctrl.h>
#include <psp2/net/net.h>

#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <inttypes.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include <unordered_map>
#include <map>
#include <numeric>
#include <functional>
// this code still messy, so im realy sorry for this unreadable code
extern "C" {
	int _newlib_heap_size_user = 256 * 1024 * 1024;
}
char* stringToCharArray(const std::string& str);
unsigned char readUTF(unsigned char c, int fd);
std::string getBodyRequest(std::string input);
size_t skipWhitespace(const std::string& input, size_t pos) {
    while (pos < input.length() && isspace(input[pos])) {
        pos++;
    }
    return pos;
}

// Function to parse a JSON value (string, number, object, array)
void parseValue(const std::string& input, size_t& pos, std::string& jsonValue);
std::string base64Encode(const std::string& input);
std::vector<std::string> splitString(std::string input, char tokenChar);
void createRequest(std::string urlTemp, std::function<void(std::string)> responseCall);
// Function to parse a JSON object and store key-value pairs in a map
void parseJsonObject(const std::string& input, size_t& pos, std::map<std::string, std::string>& jsonMap) {
    pos = skipWhitespace(input, pos);
    
    if (input[pos] == '{') {
        pos++; // Move past the opening brace
    } else {
        // std::cerr << "Error: Expected '{' at position " << pos << std::endl;
        exit(1);
    }

    while (pos < input.length()) {
        pos = skipWhitespace(input, pos);
        if (input[pos] == '}') {
            break; // End of object
        }

        // Parse key (assuming keys are enclosed in double quotes)
        if (input[pos] == '"') {
            pos++; // Move past the opening double quote
            size_t keyStart = pos;
            while (pos < input.length() && input[pos] != '"') {
                pos++;
            }

            if (pos < input.length() && input[pos] == '"') {
                std::string key = input.substr(keyStart, pos - keyStart);
                pos++; // Move past the closing double quote
                pos = skipWhitespace(input, pos);

                if (input[pos] == ':') {
                    pos++; // Move past the colon
                    pos = skipWhitespace(input, pos);

                    // Parse value
                    //  jsonMap[key] << "\n";
                    parseValue(input, pos, jsonMap[key]);
                    // std::cout << jsonMap[key] << "\n\n";
                } else {
                    // std::cerr << "Error: Expected colon after key at position " << pos << std::endl;
                    exit(1);
                }
            } else {
                // std::cerr << "Error: Expected closing double quote for key at position " << pos << std::endl;
                exit(1);
            }
        } else {
            // std::cerr << "Error: Expected double-quoted key at position " << pos << std::endl;
            exit(1);
        }

        pos = skipWhitespace(input, pos);

        if (input[pos] == ',') {
            pos++; // Move past the comma to the next key-value pair
        } else if (input[pos] == '}') {
            break; // End of object
        } else {
            // std::cerr << "Error: Expected ',' or '}' at position " << pos << std::endl;
            exit(1);
        }
    }

    if (input[pos] == '}') {
        pos++; // Move past the closing brace
    } else {
        // std::cerr << "Error: Expected '}' at the end of the object" << std::endl;
        exit(1);
    }
}

// Function to parse a JSON array and store elements in a vector
void parseJsonArray(const std::string& input, size_t& pos, std::vector<std::string>& jsonArray) {
    pos = skipWhitespace(input, pos);
    
    if (input[pos] == '[') {
        pos++; // Move past the opening bracket
    } else {
        // std::cerr << "Error: Expected '[' at position " << pos << std::endl;
        exit(1);
    }

    while (pos < input.length()) {
        pos = skipWhitespace(input, pos);
        if (input[pos] == ']') {
            break; // End of array
        }

        // Parse value
        std::string element;
        parseValue(input, pos, element);
        jsonArray.push_back(element);

        pos = skipWhitespace(input, pos);

        if (input[pos] == ',') {
            pos++; // Move past the comma to the next element
        } else if (input[pos] == ']') {
            break; // End of array
        } else {
            // std::cerr << "Error: Expected ',' or ']' at position " << pos << std::endl;
            exit(1);
        }
    }

    if (input[pos] == ']') {
        pos++; // Move past the closing bracket
    } else {
        // std::cerr << "Error: Expected ']' at the end of the array" << std::endl;
        exit(1);
    }
}

// Function to parse a JSON value (string, number, object, or array)
void parseValue(const std::string& input, size_t& pos, std::string& jsonValue) {
    pos = skipWhitespace(input, pos);
    char firstChar = input[pos];

    if (isdigit(firstChar) || firstChar == '-') {
        // Parse a number
        size_t endPos;
        jsonValue = input.substr(pos, input.find_first_not_of("0123456789.-", pos) - pos);
        pos += jsonValue.length();
    } else if (firstChar == '"') {
        // Parse a string (enclosed in double quotes)
        pos++; // Move past the opening double quote
        size_t stringStart = pos;
        while (pos < input.length() && input[pos] != '"') {
            pos++;
        }

        if (pos < input.length() && input[pos] == '"') {
            jsonValue = input.substr(stringStart, pos - stringStart);
            pos++; // Move past the closing double quote
        } else {
            // std::cerr << "Error: Expected closing double quote for string at position " << pos << std::endl;
            exit(1);
        }
    } else if (firstChar == '{') {
        // Parse a JSON object
        std::map<std::string, std::string> jsonObject;
        parseJsonObject(input, pos, jsonObject);
        jsonValue = "{";
        for (const auto& pair : jsonObject) {
            jsonValue += "\"" + pair.first + "\":\"" + pair.second + "\",";
        }
        if (!jsonValue.empty() && jsonValue.back() == ',') {
            jsonValue.pop_back(); // Remove the trailing comma
        }
        jsonValue += "}";
    } else if (firstChar == '[') {
        // Parse a JSON array
        std::vector<std::string> jsonArray;
        parseJsonArray(input, pos, jsonArray);
        jsonValue = "[";
        for (const std::string& element : jsonArray) {
            jsonValue += element + ",";
        }
        if (!jsonValue.empty() && jsonValue.back() == ',') {
            jsonValue.pop_back(); // Remove the trailing comma
        }
        jsonValue += "]";
    } else {
        // std::cerr << "Error: Invalid character in JSON value at position " << pos << std::endl;
        exit(1);
    }
}

// Callback function to handle the response
size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output) {
    size_t totalSize = size * nmemb;
    output->append(static_cast<char*>(contents), totalSize);
    return totalSize;
}
class EditorValue {
    public: std::string name = "";
    public: std::string type = "";
    public: std::vector<std::string> lineEditorText = {""};
    public: int cursorPosX = 0;
    public: int cursorPosY = 0;
};
int percentageInt = 37;
static std::vector<std::vector<char>> listKey = {
		{'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i'},
		{'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r'},
		{'s', 't', 'u', 'v', 'w', 'x', 'y', 'z', '0'},
		{'1', '2', '3', '4', '5', '6', '7', '8', '9'},
		{'[', ']', '{', '}', '(', ')', '-', '_', '!'},
		{'+', '=', '|', ';', ':', ',', '.', '<', '>'},
		{'@', '#', '$', '%', '^', '&', '*', 39, '?'},
        {'`', '~', '/', '"', '\\', 9, '.', ':', '"'}
	};
static int indexKey = 0;
static bool isCursorOn = false;
static int rowKey = 0;
static bool isCapslock = false;
static int counter = 0;
static std::vector<std::string> listAlert = {};
class CommandBlock {
public:
    CommandBlock(const std::string& initValue, const std::string& initTitle)
        : value(initValue), title(initTitle) {
    };
    std::string value;
    std::string title;
};
enum TypeWindow {
    Console,
    TextEditor,
    InputPad,
	Alert
};
class WindowResponse {
    public: bool isSelected;
};
class WindowVar {
    public: std::string idWindow;
    public: TypeWindow type;
    public: std::vector<CommandBlock> listCommand;
    public: char commandStr[128] = "";
    public: EditorValue textEdit;
    public: std::string value;
	public: bool isOpen = false;
    public: WindowVar(const std::vector<CommandBlock>& commandTemp)
        : listCommand(commandTemp) {

    };
    public: WindowVar(const bool isOpenTemp)
        : isOpen(isOpenTemp) {

    };
};
WindowResponse createWindow(TypeWindow type, WindowVar& data){
    WindowResponse responseWindow = WindowResponse();
    std::string titleId = data.idWindow;
    switch (type) {
        case TextEditor: {
            if (titleId == ""){// Handle 
                std::string templateTitle = "Text Editor";
                titleId = templateTitle+" ("+std::to_string(ImGui::GetID(templateTitle.c_str()))+")";//get random id
                data.idWindow = titleId;
            }
            ImGui::Begin(titleId.c_str(), &(data.isOpen), ImGuiWindowFlags_MenuBar);
            responseWindow.isSelected = ImGui::IsWindowFocused();
            if (ImGui::BeginMenuBar()) {
                if (ImGui::BeginMenu("File")) {
                    if (ImGui::MenuItem("Open..", "Ctrl+O")) { /* Do stuff */ }
                    if (ImGui::MenuItem("Save", "Ctrl+S"))   { 
                        // std::cout << "Saved "+data.textEdit.name << "\n";
                        std::vector<std::string> stringVector = data.textEdit.lineEditorText;
                        std::string concatenatedString = std::accumulate(
                            stringVector.begin(),
                            stringVector.end(),
                            std::string(),
                            [](const std::string& a, const std::string& b) {
                                return a + b + "\n";
                            }
                        );
                        std::string url = "/savefile/"+base64Encode(data.textEdit.name)+"/"+base64Encode(concatenatedString);
                        auto responseCall = [&](std::string response) mutable {
                            listAlert.push_back(url);
                        };
                        createRequest(url, responseCall);
                    }
                    if (ImGui::MenuItem("Close", "Ctrl+W"))  { 
                        data.isOpen = false;
                     }
                    ImGui::EndMenu();
                }
                ImGui::EndMenuBar();
            }

            ImGui::BeginGroup();
				int indexLine = 0;
				// std::cout << data.textEdit.lineEditorText.size() << "\n";
                for (std::string& valueLineStr : data.textEdit.lineEditorText) {
					std::string valueLineStrTemp = "";
					valueLineStrTemp.assign(valueLineStr);
					if(counter>=75){
						counter=0;
						isCursorOn=!isCursorOn;
					}else{
						counter+=1;
					}
					if(indexLine==data.textEdit.cursorPosY){
						if(isCursorOn) valueLineStrTemp.insert(data.textEdit.cursorPosX, "|");
					}
					ImGui::TextWrapped(valueLineStrTemp.c_str());
					indexLine+=1;
				}
			ImGui::EndGroup();
            ImGui::End();
            break;
            }
        case Alert: {
            ImGui::Begin(titleId.c_str(), &(data.isOpen));
                responseWindow.isSelected = ImGui::IsWindowFocused();
                ImGui::TextWrapped(data.value.c_str());
            ImGui::End();
            break;
        }
		case InputPad: {
            if (titleId == ""){// Handle 
                titleId = "Keyboard Pad";//get random id
                data.idWindow = titleId;
            }
            ImGui::Begin(titleId.c_str());
            responseWindow.isSelected = ImGui::IsWindowFocused();
            std::string keyStepPos = " ";
            for(int i = 0; i<9; i++){
                keyStepPos+=" ";
                if(i==indexKey){
                    keyStepPos+="+";
                }else{
                    keyStepPos+="-";
                }
            }
            // ImGui::TextWrapped(ss.str().c_str());
            ImGui::TextWrapped(keyStepPos.c_str());
            int indexKeyStrTempArr = 0;
            std::vector<std::string> keyStrVisual = {"X", "O", "A",  "D", "X", "O", "A",  "D"};
            for (std::vector<char>& keyStrTempArr : listKey) {
                std::string keyStep = "";
                if(rowKey==0){
                    if(indexKeyStrTempArr<4){
                        keyStep = keyStrVisual[indexKeyStrTempArr];
                    }else{
                        keyStep = " ";
                    }
                }else if (rowKey==1){
                    if(indexKeyStrTempArr>3){
                        keyStep = keyStrVisual[indexKeyStrTempArr];
                    }else{
                        keyStep = " ";
                    }
                }
                for (char keyStrTemp : keyStrTempArr) {
                    keyStep+=" ";
                    bool isInKeyRange = (keyStrTemp>=97 && keyStrTemp<=122);
                    char pressKey = isCapslock && isInKeyRange ? (keyStrTemp-32) : keyStrTemp;
                    std::string strKey = "";
                    switch(pressKey){
                        case 37: 
                            strKey = "%"+std::string(1, pressKey);
                            break;
                        case 9:
                            strKey = "T";
                            break;
                        default:
                            strKey = std::string(1, pressKey);
                            break;
                    }
                    keyStep+=strKey;
                }
                ImGui::TextWrapped(keyStep.c_str());
                indexKeyStrTempArr+=1;
            }
            ImGui::End();
            break;
            }
        case Console: {
            if (titleId == ""){// Handle 
                std::string templateTitle = "Console";
                titleId = templateTitle+" ("+std::to_string(ImGui::GetID(templateTitle.c_str()))+")";//get random id
                data.idWindow = titleId;
            }
            ImGui::Begin(titleId.c_str());
            responseWindow.isSelected = ImGui::IsWindowFocused();
            ImGui::BeginChild("ScrollingRegion", ImVec2(0, 200), false, ImGuiWindowFlags_HorizontalScrollbar);
            responseWindow.isSelected = responseWindow.isSelected || ImGui::IsWindowFocused();
            int orderCommand = 1;
            for (const CommandBlock& commandTmp : data.listCommand) {
                std::string titleHeader = std::to_string(orderCommand)+" "+commandTmp.title;
                if (ImGui::CollapsingHeader(titleHeader.c_str())){
                    ImGui::TextWrapped(commandTmp.value.c_str());
                }
                orderCommand+=1;
            }
            ImGui::EndChild();
            std::string commandStr = "Command["+std::to_string(data.textEdit.cursorPosX)+":"+std::to_string(strlen(data.commandStr))+"]: ";
            ImGui::InputText(commandStr.c_str(), data.commandStr, 128);
            ImGui::End();
            break;
        }
        default:
            break;
    }
    return responseWindow;
}
void createRequest(std::string urlTemp, std::function<void(std::string)> responseCall) {
    char* wttr[] = {"example.com", stringToCharArray(urlTemp)};
	char**url = wttr;

	#ifdef __vita__
		static char net_mem[1*1024*1024];
		sceSysmoduleLoadModule(SCE_SYSMODULE_NET);
		SceNetInitParam initParam;
		initParam.memory = net_mem;
		initParam.size = sizeof(net_mem);
		sceNetInit(&initParam);
	#endif

	int fd = socket(PF_INET, SOCK_STREAM, 0);
	struct sockaddr_in server_address;
	memset(&server_address, 0, sizeof(server_address));
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(80);
	server_address.sin_addr.s_addr = *(long*)(gethostbyname(url[0])->h_addr);

	connect(fd, (const struct sockaddr *)&server_address, sizeof(server_address));
	
	char*header[] = {"GET ",url[1]," HTTP/1.1\r\n", "User-Agent: curl/PSVita\r\n", "Host: ",url[0],"\r\n", "\r\n", 0};
	for(int i = 0; header[i]; i++)//send all request header to the server
		write(fd, header[i], strlen(header[i]));

	unsigned pos = 0;
	unsigned char c, line[4096];
	while(read(fd,&c,sizeof(c)) > 0 && pos < sizeof(line)) {
		if (c>>6==3) // if fetched char is a UTF8 
			c = readUTF(c,fd); // convert it back to ASCII
		line[pos] = c;
		pos++;
	}
	size_t size = sizeof(line) / sizeof(line[0]);
    std::string stringValue(line);
	close(fd);

	#ifndef __vita__ // generate a RGB screen dump (if built on PC)
	//convert the dump into PNG with: convert -depth 8 -size 960x544+0 RGB:screen.data out.png;
	int fdump = open("screen.data", O_WRONLY | O_CREAT | O_TRUNC, 0777);
	for (unsigned i = 0; i < sizeof(base); i += 4)// for every RGBA bytes pointed by the screen "base" adress
		write(fdump, base+i, 3);//write the RGB part (3 bytes) into "screen.data"
	close(fdump);
	#endif
	responseCall(getBodyRequest(stringValue));
}

unsigned char readUTF(unsigned char c, int fd){
	int uni = 0;char u;
	if(c>=0b11000000){uni = uni?:c & 0b011111;read(fd,&u,sizeof(u));uni = (uni << 6) | (u & 0x3f);}
	if(c>=0b11100000){uni = uni?:c & 0b001111;read(fd,&u,sizeof(u));uni = (uni << 6) | (u & 0x3f);}
	if(c>=0b11110000){uni = uni?:c & 0b000111;read(fd,&u,sizeof(u));uni = (uni << 6) | (u & 0x3f);}
	switch(uni) {
	case  176:return 0xBC;
	case 8213:return 0x17;
	case 8216:return 0x60;
	case 8217:return 0x27;
	case 8218:return ',';
	case 8230:return 0x2E;
	case 8592:return '<';
	case 8593:return 0xCE;
	case 8594:return '>';
	case 8595:return 0xCD;
	case 8598:return '\\';
	case 8599:return '/';
	case 8601:return '\\';
	case 8600:return '\\';
	case 8602:return '/';
	case 9472:return 0x17;
	case 9474:return 0x16;
	case 9484:return 0x18;
	case 9488:return 0x19;
	case 9492:return 0x1A;
	case 9496:return 0x1B;
	case 9500:return 0x14;
	case 9508:return 0x13;
	case 9516:return 0x12;
	case 9524:return 0x11;
	case 9532:return 0x15;
	case 9600:return 0xC0;
	case 9601:return 0xC0;
	case 9602:return 0xDC;
	case 9603:return 0xDC;
	case 9604:return 0xC2;
	case 9605:return 0xC2;
	case 9606:return 0xDB;
	case 9607:return 0xDB;
	case 9608:return 0xDB;
	}
	return '?';
}
void ModIN_RescaleAnalog(int *x, int *y, int dead) {
	float analogX = (float) *x;
	float analogY = (float) *y;
	float deadZone = (float) dead;
	float maximum = 32768.0f;
	float magnitude = sqrt(analogX * analogX + analogY * analogY);
	if (magnitude >= deadZone)
	{
		float scalingFactor = maximum / magnitude * (magnitude - deadZone) / (maximum - deadZone);
		*x = (int) (analogX * scalingFactor);
		*y = (int) (analogY * scalingFactor);
	} else {
		*x = 0;
		*y = 0;
	}
}
void Mod_ImplVitaGL_PollLeftStick(SceCtrlData *pad, int *x, int *y){
	sceCtrlPeekBufferPositive(0, pad, 1);
	*x = (pad->lx);
	*y = (pad->ly);
}

int main(int, char**)
{
	
	vglInitExtended(0, 960, 544, 0x1800000, SCE_GXM_MULTISAMPLE_4X);
	// Setup ImGui binding
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	ImGui_ImplVitaGL_Init();

	// Setup style
	ImGui::StyleColorsDark();

	bool show_demo_window = true;
	bool isGamePadUsage = false;
	bool show_another_window = false;
	
	ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

	ImGui_ImplVitaGL_TouchUsage(true);
	ImGui_ImplVitaGL_UseIndirectFrontTouch(false);
	ImGui_ImplVitaGL_UseRearTouch(true);
	ImGui_ImplVitaGL_GamepadUsage(isGamePadUsage);
	ImGui_ImplVitaGL_MouseStickUsage(isGamePadUsage);

    // Main loop
    bool done = false;
    
    bool isOpenConsole = false;
    WindowVar startConsole(&isOpenConsole);
    startConsole.idWindow = "Console 1";
    startConsole.type = Console;
    bool isGamepadOpen = false;
    WindowVar gamepadConsole(&isGamepadOpen);
    gamepadConsole.idWindow = "Gamepad";
    gamepadConsole.type = InputPad;
    std::vector<WindowVar> listWindow = {gamepadConsole, startConsole};
    
    WindowVar* selectedWindow = &startConsole;
    int* cursorPosX = 0;
    int* cursorPosY = 0;
    std::vector<std::string> lineEditorText = {""};
    // std::cout << cursorPosX << " " << lineEditorText.size() << "\n";



	SceCtrlData prevCtrl;
	sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG);
	
	while (!done){
		ImGui_ImplVitaGL_NewFrame();

		cursorPosX = &selectedWindow->textEdit.cursorPosX;
		cursorPosY = &selectedWindow->textEdit.cursorPosY;
		if(selectedWindow->type==Console){
			selectedWindow->textEdit.lineEditorText = {selectedWindow->commandStr};
		}
		lineEditorText = selectedWindow->textEdit.lineEditorText;

        SceCtrlData ctrl;
		int lstick_x, lstick_y = 0;
		Mod_ImplVitaGL_PollLeftStick(&ctrl, &lstick_x, &lstick_y);
		bool isPressKey = (ctrl.buttons & ~prevCtrl.buttons);
		bool isEnterPress = false;
		if(isPressKey){
			if (ctrl.buttons & SCE_CTRL_START) {
				isGamePadUsage=!isGamePadUsage;
				ImGui_ImplVitaGL_GamepadUsage(isGamePadUsage);
				ImGui_ImplVitaGL_MouseStickUsage(isGamePadUsage);
			}else if (ctrl.buttons & SCE_CTRL_SELECT) {
				isEnterPress = true;
			}
		}
		// int indexKey = 0;
		int xAnalogCheck = 0;
		if(isGamePadUsage==false){
			bool isInputKey = false;
			int indexKeyPad = 0;
			if(isPressKey){
				if(ctrl.buttons & SCE_CTRL_CROSS){ indexKeyPad = 1;}
				else if(ctrl.buttons & SCE_CTRL_CIRCLE){ indexKeyPad = 2;}
				else if(ctrl.buttons & SCE_CTRL_TRIANGLE){ indexKeyPad = 3;}
				else if(ctrl.buttons & SCE_CTRL_SQUARE){ indexKeyPad = 4;}
				else if(ctrl.buttons & SCE_CTRL_LTRIGGER){ indexKeyPad = 5;}
				else if(ctrl.buttons & SCE_CTRL_RTRIGGER){ indexKeyPad = 6;}
				
				// have to change to ps vita code
				// if(ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_1))){ 
				// 	if(rowKey==0){
				// 		rowKey = 1;
				// 	}else{
				// 		rowKey = 0;
				// 	}
				// }else if(ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_2))){ 
				// 	isCapslock=!isCapslock;
				// }

				int xMove = 0;
				int yMove = 0;
				if(ctrl.buttons & SCE_CTRL_LEFT){ xMove=-1; }
				if(ctrl.buttons & SCE_CTRL_RIGHT){ xMove=1; }
				if(ctrl.buttons & SCE_CTRL_UP){ yMove=-1;}
				if(ctrl.buttons & SCE_CTRL_DOWN){ yMove=1;}

				if(((*cursorPosX)+xMove)>=0 && ((*cursorPosX)+xMove)<=(lineEditorText[(*cursorPosY)].size())){
					selectedWindow->textEdit.cursorPosX+=xMove;
				}
				if(((*cursorPosY)+yMove)>=0 && ((*cursorPosY)+yMove)<lineEditorText.size()){
					selectedWindow->textEdit.cursorPosY+=yMove;
					if((*cursorPosX)>lineEditorText[(*cursorPosY)].size()){
						selectedWindow->textEdit.cursorPosX = lineEditorText[(*cursorPosY)].size();
						(*cursorPosX) = selectedWindow->textEdit.cursorPosX;
					}
				}
			}
			
			int xAnalog = 1;
			int yAnalog = 1;
			
			bool isAnalogUp = lstick_y<30;
			bool isAnalogRight = lstick_x>210;
			bool isAnalogDown = lstick_y>210;
			bool isAnalogLeft = lstick_x<30;
			if(isAnalogUp){ yAnalog = 0; }
			if(isAnalogRight){ xAnalog = 2; }
			if(isAnalogDown){ yAnalog = 2; }
			if(isAnalogLeft){ xAnalog = 0; }
			xAnalogCheck = lstick_x;
			indexKey = (yAnalog*3)+xAnalog;
			if(isPressKey){
				if(indexKeyPad>0){
					// std::string keyStr = "";
					if(indexKeyPad==6){//space
						selectedWindow->textEdit.lineEditorText[(*cursorPosY)].insert((*cursorPosX), " ");
                        selectedWindow->textEdit.cursorPosX+=1;
					}else if(indexKeyPad==5){//backspace
						std::string lineStrTemp = "";
						lineStrTemp.assign(lineEditorText[(*cursorPosY)]);
						int cursorPosXTemp = *cursorPosX;
						if(((*cursorPosX)-1)>=0){
							lineStrTemp.erase((*cursorPosX)-1, 1);
							cursorPosXTemp-=1;
						}
						selectedWindow->textEdit.lineEditorText[(*cursorPosY)] = lineStrTemp;
						selectedWindow->textEdit.cursorPosX=cursorPosXTemp;
					}else{
						int indexYKey = (rowKey*4)+(indexKeyPad-1);
						// unsigned char value[] = listKey[indexYKey][indexKey];
						// char charValue = std::string(reinterpret_cast<char*>(value), sizeof(value));
						char keyString = listKey[indexYKey][indexKey];
						if(isCapslock){
							int charIntKey = keyString;
							if(charIntKey>=97 && charIntKey<=122){
								char keyIntPress = (charIntKey-32);
								keyString = keyIntPress;
							}
						}
						std::string strKey = std::string(1, keyString);
						selectedWindow->textEdit.lineEditorText[(*cursorPosY)].insert((*cursorPosX), strKey);
						selectedWindow->textEdit.cursorPosX+=1;
					}
				}
			}
		}
		prevCtrl = ctrl;
		for (std::string& alertMassage : listAlert) {
			bool isOpenWindow = true;
			WindowVar newWindow(&isOpenWindow);
			newWindow.idWindow = "Alert "+std::to_string(listWindow.size()+1);
			newWindow.type = Alert;
			newWindow.value = alertMassage;
			listWindow.push_back(newWindow);
		}
		if(listAlert.size()>0){
			listAlert.clear();
		}
		for (WindowVar& windowItem : listWindow) {
			// WindowVar startConsole
			if(windowItem.isOpen == true){
				WindowResponse windowResponseTemp = createWindow(windowItem.type, windowItem);
				if(windowResponseTemp.isSelected){
					if (isEnterPress) {
						switch (windowItem.type) {
							case TextEditor: {
								selectedWindow->textEdit.lineEditorText.push_back({""});
								*cursorPosY+=1;
								if((*cursorPosX)>lineEditorText[(*cursorPosY)].size()){
									selectedWindow->textEdit.cursorPosX = lineEditorText[(*cursorPosY)].size();
									(*cursorPosX) = selectedWindow->textEdit.cursorPosX;
								}
								break;
							}
							case Console: {
								auto responseCall = [&](std::string response) mutable {
									CommandBlock inputCommand(response, windowItem.commandStr);
									windowItem.listCommand.push_back(inputCommand);
								};
								if(windowItem.commandStr!=""){
									std::string compareCheck = "gamepad";
									std::string compareEdit = "edit ";
									std::string compareLeetcode = "leetcode ";
									std::string compareHit = "hit";
									std::string compareConsole = "console";
									if (std::string(windowItem.commandStr).substr(0, 5)==compareEdit){
										std::string fileName = std::string(windowItem.commandStr).substr(5, strlen(windowItem.commandStr));
										auto readFileRemote = [&](std::string response) mutable {
											bool isOpenWindow = false;
											EditorValue editValue = EditorValue();
											editValue.name = fileName;
											editValue.lineEditorText = splitString(response, '\n');
											WindowVar newWindow(&isOpenWindow);
											newWindow.idWindow = "Text Editor "+fileName;
											newWindow.textEdit = editValue;
											newWindow.type = TextEditor;
											listWindow.push_back(newWindow);
										};
										std::string url = "/requestForPass/";
										url=url+base64Encode("cat "+fileName);
										createRequest(url, readFileRemote);
									}else if (windowItem.commandStr==compareConsole){
										bool isOpenWindow = true;
										WindowVar newWindow(&isOpenWindow);
										newWindow.idWindow = "Console "+std::to_string(listWindow.size()+1);
										newWindow.type = Console;
										listWindow.push_back(newWindow);
									}else if (windowItem.commandStr==compareCheck){
										bool isOpenWindow = true;
										WindowVar newWindow(&isOpenWindow);
										newWindow.idWindow = "Gamepad";
										newWindow.type = InputPad;
										listWindow.push_back(newWindow);
									}else if (std::string(windowItem.commandStr).substr(0, 3)==compareHit){
										std::string url = "/requestForPass/";
										url=url+base64Encode(std::string(windowItem.commandStr).substr(4, std::string(windowItem.commandStr).length()));
										createRequest(url, responseCall);
									}else if (std::string(windowItem.commandStr).substr(0, 8)==compareLeetcode){
										
									}
								}
								break;
							}
							default: { break; }
						}
					}
					selectedWindow = &windowItem;
				}
			}
		}
		if(selectedWindow->type==Console){
			if(selectedWindow->textEdit.lineEditorText.size()>0){
				std::string commandCheck = selectedWindow->textEdit.lineEditorText[0];
				const char* newCharArr = commandCheck.c_str();
				strcpy(selectedWindow->commandStr, newCharArr);
			}
		}

		// Rendering
		glViewport(0, 0, (int)ImGui::GetIO().DisplaySize.x, (int)ImGui::GetIO().DisplaySize.y);
		glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
		glClear(GL_COLOR_BUFFER_BIT);
		//glUseProgram(0); // You may want this if using this code in an OpenGL 3+ context where shaders may be bound
		ImGui::Render();
		ImGui_ImplVitaGL_RenderDrawData(ImGui::GetDrawData());
		vglSwapBuffers(GL_FALSE);
	}

	// Cleanup
	ImGui_ImplVitaGL_Shutdown();
	ImGui::DestroyContext();
	
	vglEnd();

	#ifdef __vita__
	sceNetTerm();
	sceSysmoduleUnloadModule(SCE_SYSMODULE_NET);
	sceKernelDelayThread(~0);
	#endif
	return 0;
}
std::string base64Encode(const std::string& input) {
    static const std::string base64Chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string encoded;
    size_t inputLength = input.length();
    for (size_t i = 0; i < inputLength; i += 3) {
        size_t group = std::min<size_t>(3, inputLength - i);
        unsigned int packed = 0;
        for (size_t j = 0; j < group; ++j) {
            packed |= static_cast<unsigned char>(input[i + j]) << ((2 - j) * 8);
        }
        for (size_t j = 0; j < group + 1; ++j) {
            encoded.push_back(base64Chars[(packed >> (6 * (3 - j))) & 0x3F]);
        }
        for (size_t j = 0; j < 3 - group; ++j) {
            encoded.push_back('=');
        }
    }

    return encoded;
}
std::vector<std::string> splitString(std::string input, char tokenChar){
    std::istringstream ss(input);
    std::string token;
    std::vector<std::string> tokens;

    while (std::getline(ss, token, tokenChar)) {
        tokens.push_back(token);
    }
    return tokens;
}
std::string getBodyRequest(std::string input){
	std::vector<std::string> paragraphs;
    size_t pos = 0;
    while ((pos = input.find("\r\n\r\n")) != std::string::npos) {
        std::string paragraph = input.substr(0, pos);
		input.erase(0, pos + 4);
		paragraphs.push_back(paragraph);
    }
	paragraphs.push_back(input);
	if(paragraphs.size()<=1){
		return "";
	}
	return paragraphs[1];
}
char* stringToCharArray(const std::string& str) {
    char* charArray = new char[str.length() + 1];
    strcpy(charArray, str.c_str());
    return charArray;
}