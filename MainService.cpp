
#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <vector>
#include <sstream>
#include "tox.h"
#include "sodium.h"
#pragma comment(lib, "libtox.dll.a")
using namespace std;

typedef struct DHT_node {
	const char *ip;
	uint16_t port;
	const char key_hex[TOX_PUBLIC_KEY_SIZE * 2 + 1];
	unsigned char key_bin[TOX_PUBLIC_KEY_SIZE];
} DHT_node;

const char *savedata_filename = "DynPa.lmc";
const char *savedata_tmp_filename = "DynPa.lmc.tmp";
HANDLE ToxCommunications = 0;
bool ChatSystemInitialized = false;
bool ToxConnected = false;
Tox *tox;
bool RebootTox = true;
int RebootToxTimer = 0;
vector<FILE *> RecvFilesPointer;
vector<string> RecvFilesNamer;
vector<uint32_t> RecvFileFileNum;
vector<uint32_t> RecvFileFriendNum;
int SendFilesUID = 0;
vector<FILE *> SendFilesPointer;
vector<uint32_t> SendFileFileNum;
vector<uint32_t> SendFileFriendNum;
string APRPath;
HANDLE hThread = 0;
bool ServiceStarted = false;
void StopChatSystem();
void bootstrap(Tox *tox);

void usleep(__int64 usec){

	HANDLE timer;
	LARGE_INTEGER ft;

	ft.QuadPart = -(10 * usec);

	timer = CreateWaitableTimer(NULL, TRUE, NULL);
	SetWaitableTimer(timer, &ft, 0, NULL, NULL, 0);
	WaitForSingleObject(timer, INFINITE);
	CloseHandle(timer);

}

static void to_hex(char *out, uint8_t *in, int size) {
	while (size--) {
		if (*in >> 4 < 0xA) {
			*out++ = '0' + (*in >> 4);
		}
		else {
			*out++ = 'A' + (*in >> 4) - 0xA;
		}

		if ((*in & 0xf) < 0xA) {
			*out++ = '0' + (*in & 0xF);
		}
		else {
			*out++ = 'A' + (*in & 0xF) - 0xA;
		}
		in++;
	}
}

void id_to_string(char *dest, uint8_t *src) {
	to_hex(dest, src, TOX_ADDRESS_SIZE);
}

wstring s2ws(const string& s){
	int len;
	int slength = (int)s.length() + 1;
	len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), slength, 0, 0);
	wchar_t* buf = new wchar_t[len];
	MultiByteToWideChar(CP_UTF8, 0, s.c_str(), slength, buf, len);
	wstring r(buf);
	delete[] buf;
	return r;
}

char from_hex(char ch){
	return isdigit(ch) ? ch - '0' : tolower(ch) - 'a' + 10;
}

DWORD WINAPI ChatService(){

	while (ServiceStarted) {
		Sleep(1000);
	}

	return 0;
}

int main(){
	HWND ConWin = GetConsoleWindow();
	SetWindowText(ConWin, L"LService");
	ShowWindow(ConWin, SW_HIDE);
	ServiceStarted = true;
	hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ChatService, NULL, 0, NULL);
	WaitForSingleObject(hThread, INFINITE);
	if (ToxConnected){
		StopChatSystem();
	}
	ServiceStarted = false;
	CloseHandle(hThread);
	FreeConsole();
	return 0;
}

Tox *create_tox(){
	string ToxFilePath = APRPath + savedata_filename;
	Tox *tox;
	struct Tox_Options options;
	tox_options_default(&options);
	FILE *f = fopen(ToxFilePath.c_str(), "rb");
	if (f) {
		fseek(f, 0, SEEK_END);
		long fsize = ftell(f);
		fseek(f, 0, SEEK_SET);
		uint8_t *savedata = (uint8_t*)malloc(sizeof(char)*fsize);
		fread(savedata, fsize, 1, f);
		fclose(f);
		options.savedata_type = TOX_SAVEDATA_TYPE_TOX_SAVE;
		options.savedata_data = savedata;
		options.savedata_length = fsize;
		tox = tox_new(&options, NULL);
		free(savedata);
	}else{
		tox = tox_new(&options, NULL);
	}
	return tox;
}

void bootstrap(Tox *tox){
	DHT_node nodes[] = {
			{ "node.tox.biribiri.org", 33445, "F404ABAA1C99A9D37D61AB54898F56793E1DEF8BD46B1038B9D822E8460FAB67", { 0 } },
			{ "nodes.tox.chat", 33445, "6FC41E2BD381D37E9748FC0E0328CE086AF9598BECC8FEB7DDF2E440475F300E", { 0 } },
			{ "130.133.110.14", 33445, "461FA3776EF0FA655F1A05477DF1B3B614F7D6B124F7DB1DD4FE3C08B03B640F", { 0 } },
			{ "2001:6f8:1c3c:babe::14:1", 33445, "461FA3776EF0FA655F1A05477DF1B3B614F7D6B124F7DB1DD4FE3C08B03B640F", { 0 } },
			{ "205.185.116.116", 33445, "A179B09749AC826FF01F37A9613F6B57118AE014D4196A0E1105A98F93A54702", { 0 } },
			{ "198.98.51.198", 33445, "1D5A5F2F5D6233058BF0259B09622FB40B482E4FA0931EB8FD3AB8E7BF7DAF6F", { 0 } },
			{ "2605:6400:1:fed5:22:45af:ec10:f329", 33445, "1D5A5F2F5D6233058BF0259B09622FB40B482E4FA0931EB8FD3AB8E7BF7DAF6F", { 0 } },
			{ "85.172.30.117", 33445, "8E7D0B859922EF569298B4D261A8CCB5FEA14FB91ED412A7603A585A25698832", { 0 } },
			{ "194.249.212.109", 33445, "3CEE1F054081E7A011234883BC4FC39F661A55B73637A5AC293DDF1251D9432B", { 0 } },
			{ "2001:1470:fbfe::109", 33445, "3CEE1F054081E7A011234883BC4FC39F661A55B73637A5AC293DDF1251D9432B", { 0 } },
			{ "185.25.116.107", 33445, "DA4E4ED4B697F2E9B000EEFE3A34B554ACD3F45F5C96EAEA2516DD7FF9AF7B43", { 0 } },
			{ "2a00:7a60:0:746b::3", 33445, "DA4E4ED4B697F2E9B000EEFE3A34B554ACD3F45F5C96EAEA2516DD7FF9AF7B43", { 0 } },
			{ "46.101.197.175", 443, "CD133B521159541FB1D326DE9850F5E56A6C724B5B8E5EB5CD8D950408E95707", { 0 } },
			{ "2a03:b0c0:3:d0::ac:5001", 443, "CD133B521159541FB1D326DE9850F5E56A6C724B5B8E5EB5CD8D950408E95707", { 0 } },
			{ "5.189.176.217", 5190, "2B2137E094F743AC8BD44652C55F41DFACC502F125E99E4FE24D40537489E32F", { 0 } },
			{ "2a02:c200:1:10:3:1:605:1337", 5190, "2B2137E094F743AC8BD44652C55F41DFACC502F125E99E4FE24D40537489E32F", { 0 } },
			{ "217.182.143.254", 2306, "7AED21F94D82B05774F697B209628CD5A9AD17E0C073D9329076A4C28ED28147", { 0 } },
			{ "2001:41d0:302:1000::e111", 2306, "7AED21F94D82B05774F697B209628CD5A9AD17E0C073D9329076A4C28ED28147", { 0 } },
			{ "104.223.122.15", 33445, "0FB96EEBFB1650DDB52E70CF773DDFCABE25A95CC3BB50FC251082E4B63EF82A", { 0 } },
			{ "2607:ff48:aa81:800::35eb:1", 33445, "0FB96EEBFB1650DDB52E70CF773DDFCABE25A95CC3BB50FC251082E4B63EF82A", { 0 } },
			{ "tox.verdict.gg", 33445, "1C5293AEF2114717547B39DA8EA6F1E331E5E358B35F9B6B5F19317911C5F976", { 0 } },
			{ "d4rk4.ru", 1813, "53737F6D47FA6BD2808F378E339AF45BF86F39B64E79D6D491C53A1D522E7039", { 0 } },
			{ "104.233.104.126", 33445, "EDEE8F2E839A57820DE3DA4156D88350E53D4161447068A3457EE8F59F362414", { 0 } },
			{ "51.254.84.212", 33445, "AEC204B9A4501412D5F0BB67D9C81B5DB3EE6ADA64122D32A3E9B093D544327D", { 0 } },
			{ "2001:41d0:a:1a3b::18", 33445, "AEC204B9A4501412D5F0BB67D9C81B5DB3EE6ADA64122D32A3E9B093D544327D", { 0 } },
			{ "88.99.133.52", 33445, "2D320F971EF2CA18004416C2AAE7BA52BF7949DB34EA8E2E21AF67BD367BE211", { 0 } },
			{ "92.54.84.70", 33445, "5625A62618CB4FCA70E147A71B29695F38CC65FF0CBD68AD46254585BE564802", { 0 } },
			{ "tox.uplinklabs.net", 33445, "1A56EA3EDF5DF4C0AEABBF3C2E4E603890F87E983CAC8A0D532A335F2C6E3E1F", { 0 } },
			{ "toxnode.nek0.net", 33445, "20965721D32CE50C3E837DD75B33908B33037E6225110BFF209277AEAF3F9639", { 0 } },
			{ "95.215.44.78", 33445, "672DBE27B4ADB9D5FB105A6BB648B2F8FDB89B3323486A7A21968316E012023C", { 0 } },
			{ "2a02:7aa0:1619::c6fe:d0cb", 33445, "672DBE27B4ADB9D5FB105A6BB648B2F8FDB89B3323486A7A21968316E012023C", { 0 } },
			{ "163.172.136.118", 33445, "2C289F9F37C20D09DA83565588BF496FAB3764853FA38141817A72E3F18ACA0B", { 0 } },
			{ "2001:bc8:4400:2100::1c:50f", 33445, "2C289F9F37C20D09DA83565588BF496FAB3764853FA38141817A72E3F18ACA0B", { 0 } },
			{ "sorunome.de", 33445, "02807CF4F8BB8FB390CC3794BDF1E8449E9A8392C5D3F2200019DA9F1E812E46", { 0 } },
			{ "37.97.185.116", 33445, "E59A0E71ADA20D35BD1B0957059D7EF7E7792B3D680AE25C6F4DBBA09114D165", { 0 } },
			{ "193.124.186.205", 5228, "9906D65F2A4751068A59D30505C5FC8AE1A95E0843AE9372EAFA3BAB6AC16C2C", { 0 } },
			{ "2a02:f680:1:1100::542a", 5228, "9906D65F2A4751068A59D30505C5FC8AE1A95E0843AE9372EAFA3BAB6AC16C2C", { 0 } },
			{ "80.87.193.193", 33445, "B38255EE4B054924F6D79A5E6E5889EC94B6ADF6FE9906F97A3D01E3D083223A", { 0 } },
			{ "2a01:230:2:6::46a8", 33445, "B38255EE4B054924F6D79A5E6E5889EC94B6ADF6FE9906F97A3D01E3D083223A", { 0 } },
			{ "initramfs.io", 33445, "3F0A45A268367C1BEA652F258C85F4A66DA76BCAA667A49E770BCC4917AB6A25", { 0 } },
			{ "hibiki.eve.moe", 33445, "D3EB45181B343C2C222A5BCF72B760638E15ED87904625AAD351C594EEFAE03E", { 0 } },
			{ "tox.deadteam.org", 33445, "C7D284129E83877D63591F14B3F658D77FF9BA9BA7293AEB2BDFBFE1A803AF47", { 0 } },
			{ "46.229.52.198", 33445, "813C8F4187833EF0655B10F7752141A352248462A567529A38B6BBF73E979307", { 0 } },
			{ "node.tox.ngc.network", 33445, "A856243058D1DE633379508ADCAFCF944E40E1672FF402750EF712E30C42012A", { 0 } },
			{ "149.56.140.5", 33445, "7E5668E0EE09E19F320AD47902419331FFEE147BB3606769CFBE921A2A2FD34C", { 0 } },
			{ "2607:5300:0201:3100:0000:0000:0000:3ec2", 33445, "7E5668E0EE09E19F320AD47902419331FFEE147BB3606769CFBE921A2A2FD34C", { 0 } },
			{ "185.14.30.213", 443, "2555763C8C460495B14157D234DD56B86300A2395554BCAE4621AC345B8C1B1B", { 0 } },
			{ "2a00:1ca8:a7::e8b", 443, "2555763C8C460495B14157D234DD56B86300A2395554BCAE4621AC345B8C1B1B", { 0 } },
			{ "85.21.144.224", 33445, "8F738BBC8FA9394670BCAB146C67A507B9907C8E564E28C2B59BEBB2FF68711B", { 0 } },
			{ "tox.natalenko.name", 33445, "1CB6EBFD9D85448FA70D3CAE1220B76BF6FCE911B46ACDCF88054C190589650B", { 0 } },
			{ "37.187.122.30", 33445, "BEB71F97ED9C99C04B8489BB75579EB4DC6AB6F441B603D63533122F1858B51D", { 0 } },
			{ "136.243.141.187", 443, "6EE1FADE9F55CC7938234CC07C864081FC606D8FE7B751EDA217F268F1078A39", { 0 } },
			{ "2a01:4f8:212:2459::a:1337", 443, "6EE1FADE9F55CC7938234CC07C864081FC606D8FE7B751EDA217F268F1078A39", { 0 } },
			{ "tox.abilinski.com", 33445, "0E9D7FEE2AA4B42A4C18FE81C038E32FFD8D907AAA7896F05AA76C8D31A20065", { 0 } },
			{ "95.215.46.114", 33445, "5823FB947FF24CF83DDFAC3F3BAA18F96EA2018B16CC08429CB97FA502F40C23", { 0 } },
			{ "2a02:7aa0:1619::bdbd:17b8", 33445, "5823FB947FF24CF83DDFAC3F3BAA18F96EA2018B16CC08429CB97FA502F40C23", { 0 } },
			{ "m.loskiq.it", 33445, "88124F3C18C6CFA8778B7679B7329A333616BD27A4DFB562D476681315CF143D", { 0 } },
			{ "192.99.232.158", 33445, "7B6CB208C811DEA8782711CE0CAD456AAC0C7B165A0498A1AA7010D2F2EC996C", { 0 } },
			{ "192.99.232.158", 33445, "7B6CB208C811DEA8782711CE0CAD456AAC0C7B165A0498A1AA7010D2F2EC996C", { 0 } },
			{ "tmux.ru", 33445, "7467AFA626D3246343170B309BA5BDC975DF3924FC9D7A5917FBFA9F5CD5CD38", { 0 } },
			{ "37.48.122.22", 33445, "1B5A8AB25FFFB66620A531C4646B47F0F32B74C547B30AF8BD8266CA50A3AB59", { 0 } },
			{ "2001:1af8:4700:a115:6::b", 33445, "1B5A8AB25FFFB66620A531C4646B47F0F32B74C547B30AF8BD8266CA50A3AB59", { 0 } },
			{ "tox.novg.net", 33445, "D527E5847F8330D628DAB1814F0A422F6DC9D0A300E6C357634EE2DA88C35463", { 0 } },
			{ "tox.dumalogiya.ru", 33445, "2DAE6EB8C16131761A675D7C723F618FBA9D29DD8B4E0A39E7E3E8D7055EF113", { 0 } },
			{ "t0x-node1.weba.ru", 33445, "5A59705F86B9FC0671FDF72ED9BB5E55015FF20B349985543DDD4B0656CA1C63", { 0 } },
			{ "109.195.99.39", 33445, "EF937F61B4979B60BBF306752D8F32029A2A05CD2615B2E9FBFFEADD8E7D5032", { 0 } },
			{ "79.140.30.52", 33445, "FFAC871E85B1E1487F87AE7C76726AE0E60318A85F6A1669E04C47EB8DC7C72D", { 0 } },
			{ "94.41.167.70", 33445, "E519B2C1098999B60190012C7B53E8C43A73C535721036CD9DEC7CCA06741A7D", { 0 } },
			{ "104.223.122.204", 33445, "3925752E43BF2F8EB4E12B0E9414311064FF2D76707DC7D5D2CCB43F75081F6B", { 0 } },
			{ "77.55.211.53", 53, "B9D109CC820C69A5D97A4A1A15708107C6BA85C13BC6188CC809D374AFF18E63", { 0 } },
			{ "boseburo.ddns.net", 33445, "AF3FC9FC3D121E82E362B4FA84A53E63F58C11C2BA61D988855289B8CABC9B18", { 0 } }
	};
	for (size_t i = 0; i < sizeof(nodes) / sizeof(DHT_node); i++) {
		sodium_hex2bin(nodes[i].key_bin, sizeof(nodes[i].key_bin), nodes[i].key_hex, sizeof(nodes[i].key_hex) - 1, NULL, NULL, NULL);
		tox_bootstrap(tox, nodes[i].ip, nodes[i].port, nodes[i].key_bin, NULL);
	}
}

void update_savedata_file(const Tox *tox){
	string ToxFilePath = APRPath + savedata_filename;
	string ToxFileTmpPath = APRPath + savedata_tmp_filename;
	size_t size = tox_get_savedata_size(tox);
	uint8_t *savedata = (uint8_t*)malloc(sizeof(uint8_t)*size);
	tox_get_savedata(tox, savedata);
	FILE *f = fopen(ToxFilePath.c_str(), "wb");
	if (f) {
		fwrite(savedata, size, 1, f);
		fclose(f);
	}else{
		FILE *f = fopen(ToxFileTmpPath.c_str(), "wb");
		if (f) {
			fwrite(savedata, size, 1, f);
			fclose(f);
			rename(ToxFileTmpPath.c_str(), ToxFilePath.c_str());
		}
	}
	free(savedata);
}

DWORD WINAPI EnableChatSysComs(){
	while (ToxConnected) {
		tox_iterate(tox, NULL);
		usleep(tox_iteration_interval(tox) * 1000);
		if (RebootTox){
			if (RebootToxTimer < 30){
				RebootToxTimer++;
			}else{
				bootstrap(tox);
				RebootToxTimer = 0;
			}
		}
	}
	return 0;
}

void self_connection_status_cb(Tox *tox, TOX_CONNECTION connection_status, void *user_data){
	string ConStatus = "";
	RebootTox = false;
	RebootToxTimer = 0;
	switch (connection_status) {
	case TOX_CONNECTION_NONE:
		ConStatus = "Offline";
		break;
	case TOX_CONNECTION_TCP:
		ConStatus = "onTCP";
		break;
	case TOX_CONNECTION_UDP:
		ConStatus = "onUDP";
		break;
	}
}

void friend_request_cb(Tox *tox, const uint8_t *public_key, const uint8_t *message, size_t length, void *user_data){
	char tox_id_hex[TOX_ADDRESS_SIZE * 2 + 1];
	id_to_string(tox_id_hex, (uint8_t*)public_key);
	string AddFriendID(tox_id_hex);
	string AddMsg(message, message + length);
}

void friend_message_cb(Tox *tox, uint32_t friend_number, TOX_MESSAGE_TYPE type, const uint8_t *message, size_t length, void *user_data){
	string Unencoded(message, message + length);
}

void friend_name_cb(Tox *tox, uint32_t friend_number, const uint8_t *name, size_t length, void *user_data){
	string NickNamer(name, name + length);
}

void friend_status_message_cb(Tox *tox, uint32_t friend_number, const uint8_t *message, size_t length, void *user_data){
	string Status(message, message + length);
}

void friend_status_cb(Tox *tox, uint32_t friend_number, TOX_USER_STATUS status, void *user_data){
	string NewStatus = "";
	switch (status) {
	case TOX_USER_STATUS_NONE:
		NewStatus = "Available";
		break;
	case TOX_USER_STATUS_AWAY:
		NewStatus = "Away";
		break;
	case TOX_USER_STATUS_BUSY:
		NewStatus = "Busy";
		break;
	}
}

void friend_connection_status_cb(Tox *tox, uint32_t friend_number, TOX_CONNECTION connection_status, void *user_data){
	string ConnectionStatus = "";
	switch (connection_status) {
	case TOX_CONNECTION_NONE:
		ConnectionStatus = "Offline";
		break;
	case TOX_CONNECTION_TCP:
		ConnectionStatus = "onTCP";
		break;
	case TOX_CONNECTION_UDP:
		ConnectionStatus = "onUDP";
		break;
	}
}

void friend_typing_cb(Tox *tox, uint32_t friend_number, bool is_typing, void *user_data){
	string TypeStatus = "";
	if (is_typing){
		TypeStatus = "ON";
	}else{
		TypeStatus = "OFF";
	}
}

void friend_read_receipt_cb(Tox *tox, uint32_t friend_number, uint32_t message_id, void *user_data){
	string FrNum = to_string(friend_number).c_str();
	string FrMsgID = to_string(message_id).c_str();
}

void file_recv_cb(Tox *tox, uint32_t friend_number, uint32_t file_number, uint32_t kind, uint64_t file_size, const uint8_t *filename, size_t filename_length, void *user_data){
	if (file_size > 0){
		string Filename(filename, filename + filename_length);
		FILE *pFile;
		RecvFilesNamer.push_back("");
		RecvFileFileNum.push_back(file_number);
		RecvFileFriendNum.push_back(friend_number);
		RecvFilesPointer.push_back(pFile);
	
	}
}

void file_recv_chunk_cb(Tox *tox, uint32_t friend_number, uint32_t file_number, uint64_t position, const uint8_t *data, size_t length, void *user_data){
	string FileStatus = "";
	int FlashFileID;
	int drf;
	int EndLen = RecvFileFileNum.size();
	for (drf = 0; drf <= EndLen - 1; drf++){
		if (RecvFileFileNum[drf] == file_number){
			if (RecvFileFriendNum[drf] == friend_number){
				FlashFileID = drf;
			}
		}
	}
	if (length <= 0){
		fclose(RecvFilesPointer[FlashFileID]);

		FileStatus = "Recieved";
	}else{

		fwrite(data, sizeof(uint8_t), length, RecvFilesPointer[FlashFileID]);
		FileStatus = "Downloading";
	}
}

void file_chunk_request_cb(Tox *tox, uint32_t friend_number, uint32_t file_number, uint64_t position, size_t length, void *user_data){
	string FileStatus = "";
	int FlashFileID;
	int dsf;
	int EndLen = SendFileFileNum.size();
	for (dsf = 0; dsf < EndLen; dsf++){
		if (SendFileFileNum[dsf] == file_number){
			if (SendFileFriendNum[dsf] == friend_number){
				FlashFileID = dsf;
			}
		}
	}
	if (length == 0){
		fclose(SendFilesPointer[FlashFileID]);
		FileStatus = "Sended";
	}else{
		fseek(SendFilesPointer[FlashFileID], (long)position, SEEK_SET);
		uint8_t *data;
		data = (uint8_t *)alloca(sizeof(uint8_t) * length);
		int len = fread(data, 1, length, SendFilesPointer[FlashFileID]);
		tox_file_send_chunk(tox, friend_number, file_number, position, data, len, NULL);
		FileStatus = "Sending";
	}
}

void file_recv_control_cb(Tox *tox, uint32_t friend_number, uint32_t file_number, TOX_FILE_CONTROL control, void *user_data){
	string SendControlStatus = "";
	string RecSend = "";
	int FlashFileID = -1;
	int drf;
	int EndLen = RecvFileFileNum.size();
	for (drf = 0; drf < EndLen; drf++){
		if (RecvFileFileNum[drf] == file_number){
			if (RecvFileFriendNum[drf] == friend_number){
				RecSend = "Recieve";
				FlashFileID = drf;
			}
		}
	}
	if (FlashFileID == -1){
		int dsf;
		int EndLen = SendFileFileNum.size();
		for (dsf = 0; dsf < EndLen; dsf++){
			if (SendFileFileNum[dsf] == file_number){
				if (SendFileFriendNum[dsf] == friend_number){
					RecSend = "Send";
					FlashFileID = dsf;
				}
			}
		}
	}
	switch (control) {
	case TOX_FILE_CONTROL_RESUME:
		SendControlStatus = "Accepted";
		break;
	case TOX_FILE_CONTROL_PAUSE:
		SendControlStatus = "Paused";
		break;
	case TOX_FILE_CONTROL_CANCEL:
		SendControlStatus = "Rejected";
		if (FlashFileID >= 0){
			if (RecSend == "Recieve"){
				fclose(RecvFilesPointer[FlashFileID]);
				remove(RecvFilesNamer[FlashFileID].c_str());
			}
			else if (RecSend == "Send"){
				fclose(SendFilesPointer[FlashFileID]);
			}
		}
		break;
	}
}

void StartChatConnections(){
	tox_callback_file_recv(tox, file_recv_cb);
	tox_callback_file_recv_chunk(tox, file_recv_chunk_cb);
	tox_callback_file_recv_control(tox, file_recv_control_cb);
	tox_callback_file_chunk_request(tox, file_chunk_request_cb);
	tox_callback_friend_request(tox, friend_request_cb);
	tox_callback_friend_name(tox, friend_name_cb);
	tox_callback_friend_message(tox, friend_message_cb);
	tox_callback_friend_read_receipt(tox, friend_read_receipt_cb);
	tox_callback_friend_status_message(tox, friend_status_message_cb);
	tox_callback_friend_status(tox, friend_status_cb);
	tox_callback_friend_connection_status(tox, friend_connection_status_cb);
	tox_callback_friend_typing(tox, friend_typing_cb);
	tox_callback_self_connection_status(tox, self_connection_status_cb);
	update_savedata_file(tox);
	ToxCommunications = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)EnableChatSysComs, NULL, 0, NULL);
}

void StartChatSystem(string NickName, string Status, string AppPath){
	APRPath = AppPath;
	bool ToxOk = false;
	if (ChatSystemInitialized == false){
		tox = create_tox();
		const uint8_t * Bname = reinterpret_cast<const uint8_t*>(NickName.c_str());
		if (tox_self_set_name(tox, Bname, NickName.length(), NULL)){
			ToxOk = true;
			const uint8_t * StatMSG = reinterpret_cast<const uint8_t*>(Status.c_str());
			if (tox_self_set_status_message(tox, StatMSG, Status.length(), NULL)){
				ToxOk = true;
			}else{
				ToxOk = false;
			}
		}else{
			ToxOk = false;
		}
	}
	if (ToxOk == true){
		ToxConnected = true;
		ChatSystemInitialized = true;
		bootstrap(tox);
		StartChatConnections();
	}else{
		tox_kill(tox);
	}
}

void SetChatNickName(string NickName){
	bool ToxOk = false;
	if (ChatSystemInitialized == true){
		const uint8_t * Bname = reinterpret_cast<const uint8_t*>(NickName.c_str());
		if (tox_self_set_name(tox, Bname, NickName.length(), NULL)){
			update_savedata_file(tox);
			ToxOk = true;
		}else{
			ToxOk = false;
		}
	}
}

void SetChatStatusMsg(string Status){
	bool ToxOk = false;
	if (ChatSystemInitialized == true){
		const uint8_t * StatMSG = reinterpret_cast<const uint8_t*>(Status.c_str());
		if (tox_self_set_status_message(tox, StatMSG, Status.length(), NULL)){
			update_savedata_file(tox);
			ToxOk = true;
		}else{
			ToxOk = false;
		}
	}
}

void SetChatConStatus(string NewConStatus){
	if (ChatSystemInitialized == true){
		if (NewConStatus == "SetOnline"){
			tox_self_set_status(tox, TOX_USER_STATUS_NONE);
		}else if (NewConStatus == "SetAway"){
			tox_self_set_status(tox, TOX_USER_STATUS_AWAY);
		}else if (NewConStatus == "SetBusy"){
			tox_self_set_status(tox, TOX_USER_STATUS_BUSY);
		}
	}
}

void SetChatTypingOnOff(string State, string FriendID){
	if (ChatSystemInitialized == true){
		uint32_t FrID = atoi(FriendID.c_str());
		if (State == "Enable"){
			tox_self_set_typing(tox, FrID, true, NULL);
		}else if (State == "Disable"){
			tox_self_set_typing(tox, FrID, false, NULL);
		}
	}
}

void GetChatConStatus(){
	string Status;
	if (ChatSystemInitialized == true){
		TOX_USER_STATUS GetConStatus = tox_self_get_status(tox);
		switch (GetConStatus){
		case TOX_USER_STATUS_NONE:
			Status = "Available";
			break;
		case TOX_USER_STATUS_AWAY:
			Status = "Away";
			break;
		case TOX_USER_STATUS_BUSY:
			Status = "Busy";
			break;
		}
	}
}

void GetChatPublicID(){
	if (ChatSystemInitialized == true){
		uint8_t tox_id_bin[TOX_ADDRESS_SIZE];
		tox_self_get_public_key(tox, tox_id_bin);
		char tox_id_hex[TOX_ADDRESS_SIZE * 2 + 1];
		sodium_bin2hex(tox_id_hex, sizeof(tox_id_hex), tox_id_bin, sizeof(tox_id_bin));
		for (size_t i = 0; i < sizeof(tox_id_hex) - 1; i++) {
			tox_id_hex[i] = toupper(tox_id_hex[i]);
		}
	}
}

void GetChatID(){
	if (ChatSystemInitialized == true){
		uint8_t tox_id_bin[TOX_ADDRESS_SIZE];
		tox_self_get_address(tox, tox_id_bin);
		char tox_id_hex[TOX_ADDRESS_SIZE * 2 + 1];
		sodium_bin2hex(tox_id_hex, sizeof(tox_id_hex), tox_id_bin, sizeof(tox_id_bin));
		for (size_t i = 0; i < sizeof(tox_id_hex) - 1; i++) {
			tox_id_hex[i] = toupper(tox_id_hex[i]);
		}
	}

}

void GetChatNoSpamID(){
	if (ChatSystemInitialized == true){
		uint32_t ToxSpamID = tox_self_get_nospam(tox);
	}
}

void GetChatSecretID(){
	if (ChatSystemInitialized == true){
		uint8_t tox_id_bin[TOX_ADDRESS_SIZE];
		tox_self_get_secret_key(tox, tox_id_bin);
		char tox_id_hex[TOX_ADDRESS_SIZE * 2 + 1];
		sodium_bin2hex(tox_id_hex, sizeof(tox_id_hex), tox_id_bin, sizeof(tox_id_bin));
		for (size_t i = 0; i < sizeof(tox_id_hex) - 1; i++) {
			tox_id_hex[i] = toupper(tox_id_hex[i]);
		}
	}
}

void GetFriendList(){
	string FrListTMP = "";
	string FrID;
	string FrToxID;
	if (ChatSystemInitialized == true){
		size_t FrListSize = tox_self_get_friend_list_size(tox);
		if (FrListSize > 0){
			for (size_t x = 0; x < FrListSize; x++){
				if (tox_friend_exists(tox, x)){
					FrID = to_string(x);
					uint8_t pk[TOX_ADDRESS_SIZE];
					if (tox_friend_get_public_key(tox, x, pk, NULL)){
						char pk_hex[TOX_ADDRESS_SIZE * 2 + 1];
						sodium_bin2hex(pk_hex, sizeof(pk_hex), pk, sizeof(pk));
						for (size_t i = 0; i < sizeof(pk_hex) - 1; i++) {
							pk_hex[i] = toupper(pk_hex[i]);
						}
						FrToxID = pk_hex;
						FrListTMP += FrID + FrToxID;
					}else{
						FrListTMP += "undefined";
					}
				}
			}
		}else{
			FrListTMP = "Empty";
		}
	}else{
		FrListTMP = "Empty";
	}
}

void SendChatMessage(string MSG, string FriendID){
	string ChatMsg;
	uint32_t FrID = atoi(FriendID.c_str());
	TOX_ERR_FRIEND_SEND_MESSAGE MSGerror;
	const uint8_t * MSGs = reinterpret_cast<const uint8_t*>(MSG.c_str());
	uint32_t MSGsID = tox_friend_send_message(tox, FrID, TOX_MESSAGE_TYPE_NORMAL, MSGs, MSG.length(), &MSGerror);
	string MsgIDsend = to_string(MSGsID);
	switch (MSGerror) {
	case TOX_ERR_FRIEND_SEND_MESSAGE_OK:
		ChatMsg = MsgIDsend.c_str();
		break;
	case TOX_ERR_FRIEND_SEND_MESSAGE_NULL:
		ChatMsg = "Null Argument";
		break;
	case TOX_ERR_FRIEND_SEND_MESSAGE_FRIEND_NOT_FOUND:
		ChatMsg = "Wrong Friend ID";
		break;
	case TOX_ERR_FRIEND_SEND_MESSAGE_FRIEND_NOT_CONNECTED:
		ChatMsg = "Not Connected With Friend";
		break;
	case TOX_ERR_FRIEND_SEND_MESSAGE_SENDQ:
		ChatMsg = "Memory Error";
		break;
	case TOX_ERR_FRIEND_SEND_MESSAGE_TOO_LONG:
		ChatMsg = "Message Exceeded Maximum";
		break;
	case TOX_ERR_FRIEND_SEND_MESSAGE_EMPTY:
		ChatMsg = "Zero Length Message";
		break;
	}
}

void SendChatInvStatus(string MSG, string FriendID){
	uint32_t FrID = atoi(FriendID.c_str());
	const uint8_t * MSGs = reinterpret_cast<const uint8_t*>(MSG.c_str());
	uint32_t MSGsID = tox_friend_send_message(tox, FrID, TOX_MESSAGE_TYPE_ACTION, MSGs, MSG.length(), NULL);
}

void SendChatFile(string SendFileName, string FriendID){
	string Chatfile;
	uint32_t FrID = atoi(FriendID.c_str());
	FILE *pFile = fopen(SendFileName.c_str(), "rb");
	SendFilesPointer.push_back(pFile);
	SendFileFileNum.push_back(NULL);
	SendFileFriendNum.push_back(FrID);
	fseek(pFile, 0, SEEK_END);
	uint64_t FileSize = ftell(pFile);
	fseek(pFile, 0, SEEK_SET);
	uint32_t TheSendedFileNumber = tox_file_send(tox, FrID, TOX_FILE_KIND_DATA, FileSize, NULL, (uint8_t *)SendFileName.c_str(), strlen(SendFileName.c_str()), 0);
	if (TheSendedFileNumber != UINT32_MAX){
		SendFileFileNum[SendFilesUID] = TheSendedFileNumber;
		tox_file_control(tox, FrID, TheSendedFileNumber, TOX_FILE_CONTROL_PAUSE, NULL);
		string MsgIDsend = to_string(SendFilesUID);
		Chatfile = MsgIDsend.c_str();
	}
	SendFilesUID++;
}

void AddChatFriend(string ChatFullID, string AddMSG){
	string AddFriend;
	char temp_id[128];
	strcpy(temp_id, ChatFullID.c_str());
	unsigned char bin_id[TOX_ADDRESS_SIZE];
	sodium_hex2bin(bin_id, sizeof(bin_id), temp_id, sizeof(temp_id), NULL, NULL, NULL);
	const uint8_t * TheFriendAddMsg = reinterpret_cast<const uint8_t*>(AddMSG.c_str());
	TOX_ERR_FRIEND_ADD AddingError;
	uint32_t FriendAddedNum = tox_friend_add(tox, bin_id, TheFriendAddMsg, AddMSG.length(), &AddingError);
	if (FriendAddedNum != UINT32_MAX){
		AddFriend = to_string(FriendAddedNum).c_str();
		update_savedata_file(tox);
	}else{
		switch (AddingError) {
		case TOX_ERR_FRIEND_ADD_OK:
			AddFriend = to_string(FriendAddedNum).c_str();
			update_savedata_file(tox);//
			break;
		case TOX_ERR_FRIEND_ADD_NULL:
			AddFriend = "Null Argument";
			break;
		case TOX_ERR_FRIEND_ADD_TOO_LONG:
			AddFriend = "Message Too Long";
			break;
		case TOX_ERR_FRIEND_ADD_NO_MESSAGE:
			AddFriend = "Message Was Empty";
			break;
		case TOX_ERR_FRIEND_ADD_OWN_KEY:
			AddFriend = "You Cant Add Yourself";
			break;
		case TOX_ERR_FRIEND_ADD_ALREADY_SENT:
			AddFriend = "Allready Friend or Send";
			break;
		case TOX_ERR_FRIEND_ADD_BAD_CHECKSUM:
			AddFriend = "Checksum Failed";
			break;
		case TOX_ERR_FRIEND_ADD_SET_NEW_NOSPAM:
			AddFriend = "Spam Value Changed";
			break;
		case TOX_ERR_FRIEND_ADD_MALLOC:
			AddFriend = "Memory Error";
			break;
		}
	}
}

void DelChatFriend(string FriendID){
	uint32_t FrID = atoi(FriendID.c_str());
	if (tox_friend_delete(tox, FrID, NULL) == true){
		update_savedata_file(tox);
	}
}

void AcceptChatFriend(string ChatID){
	string AccFriend;
	char temp_id[128];
	strcpy(temp_id, ChatID.c_str());
	unsigned char bin_id[TOX_ADDRESS_SIZE];
	sodium_hex2bin(bin_id, sizeof(bin_id), temp_id, sizeof(temp_id), NULL, NULL, NULL);
	
	TOX_ERR_FRIEND_ADD AddingError;
	uint32_t NewFriendID = tox_friend_add_norequest(tox, bin_id, &AddingError);
	if (NewFriendID != UINT32_MAX){
		AccFriend = to_string(NewFriendID).c_str();
		update_savedata_file(tox);
	}else{
		switch (AddingError) {
		case TOX_ERR_FRIEND_ADD_OK:
			AccFriend = to_string(NewFriendID).c_str();
			update_savedata_file(tox);
			break;
		case TOX_ERR_FRIEND_ADD_NULL:
			AccFriend = "Null Argument";
			break;
		case TOX_ERR_FRIEND_ADD_TOO_LONG:
			AccFriend = "Message Too Long";
			break;
		case TOX_ERR_FRIEND_ADD_NO_MESSAGE:
			AccFriend = "Message Was Empty";
			break;
		case TOX_ERR_FRIEND_ADD_OWN_KEY:
			AccFriend = "You Cant Add Yourself";
			break;
		case TOX_ERR_FRIEND_ADD_ALREADY_SENT:
			AccFriend = "Allready Friend or Send";
			break;
		case TOX_ERR_FRIEND_ADD_BAD_CHECKSUM:
			AccFriend = "Checksum Failed";
			break;
		case TOX_ERR_FRIEND_ADD_SET_NEW_NOSPAM:
			AccFriend = "Spam Value Changed";
			break;
		case TOX_ERR_FRIEND_ADD_MALLOC:
			AccFriend = "Memory Error";
			break;
		}
	}
}

void FileRecControl(string Action, string FriendID, string FileID, string FileName, string RS, string SF){
	int FlashFileID = atoi(FileID.c_str());
	if (Action == "Accept"){
		wstring RecFold = s2ws(SF);
		string RFold(RecFold.begin(), RecFold.end());
		RFold += "\\";
		RFold += FileName.c_str();
		RecvFilesNamer[FlashFileID] = FileName;
		RecvFilesPointer[FlashFileID] = fopen(RFold.c_str(), "wb");
		tox_file_control(tox, RecvFileFriendNum[FlashFileID], RecvFileFileNum[FlashFileID], TOX_FILE_CONTROL_RESUME, NULL);
	}else if (Action == "Decline"){
		tox_file_control(tox, RecvFileFriendNum[FlashFileID], RecvFileFileNum[FlashFileID], TOX_FILE_CONTROL_CANCEL, NULL);
	}else if (Action == "Pause"){
		if (RS == "Send"){
			tox_file_control(tox, SendFileFriendNum[FlashFileID], SendFileFileNum[FlashFileID], TOX_FILE_CONTROL_PAUSE, NULL);
		}else if (RS == "Recieve"){
			tox_file_control(tox, RecvFileFriendNum[FlashFileID], RecvFileFileNum[FlashFileID], TOX_FILE_CONTROL_PAUSE, NULL);
		}
	}else if (Action == "Resume"){
		if (RS == "Send"){
			tox_file_control(tox, SendFileFriendNum[FlashFileID], SendFileFileNum[FlashFileID], TOX_FILE_CONTROL_RESUME, NULL);
		}else if (RS == "Recieve"){
			tox_file_control(tox, RecvFileFriendNum[FlashFileID], RecvFileFileNum[FlashFileID], TOX_FILE_CONTROL_RESUME, NULL);
		}
	}else if (Action == "Cancel"){
		if (RS == "Send"){
			tox_file_control(tox, SendFileFriendNum[FlashFileID], SendFileFileNum[FlashFileID], TOX_FILE_CONTROL_CANCEL, NULL);
			fclose(SendFilesPointer[FlashFileID]);
		}else if (RS == "Recieve"){
			tox_file_control(tox, RecvFileFriendNum[FlashFileID], RecvFileFileNum[FlashFileID], TOX_FILE_CONTROL_CANCEL, NULL);
			fclose(RecvFilesPointer[FlashFileID]);
			remove(RecvFilesNamer[FlashFileID].c_str());
		}
	}
}

void StopChatSystem(){
	update_savedata_file(tox);
	CloseHandle(ToxCommunications);
	ToxConnected = false;
	ChatSystemInitialized = false;
	RebootTox = false;
	RebootToxTimer = 0;
	tox_kill(tox);
}
