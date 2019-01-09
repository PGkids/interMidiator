
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <mmsystem.h>

HMIDIOUT hmo;
HMIDIIN hmi;


/* DEVICE LIST */

void list_input_devices()
{
	int num, id, i;
	char pname[65];
	num = midiInGetNumDevs();
	printf("INPUT : %d\n", num);
	for(id=0; id<num; id++){
		MIDIINCAPS in;
		midiInGetDevCaps(id, &in, sizeof(in));
		memset(&pname, '\0', sizeof(pname));
		for (i=0; i<sizeof(pname); i++) {
			memcpy(pname+i, in.szPname+i, 1);
		}
		printf("[%d] %s\n", id, pname);
	}
}

void list_output_devices()
{
	int num, id, i;
	char pname[65];
	num = midiOutGetNumDevs();
	printf("OUTPUT : %d\n", num);
	for(id=0; id<num; id++){
		MIDIOUTCAPS out;
		memset(&out, '\0', sizeof(out));
		midiOutGetDevCaps(id, &out, sizeof(out));
		memset(&pname, '\0', sizeof(pname));
		for (i=0; i<sizeof(pname); i++) {
			memcpy(pname+i, out.szPname+i, 1);
		}
		printf("[%d] %s\n", id, pname);
	}
}


/* INPUT CALLBACK */

void CALLBACK MidiInProc(HMIDIIN hmi, UINT wMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2)
{
	switch (wMsg) {
	case MIM_DATA:
	case MIM_LONGDATA:
		printf("MidiInProc: wMsg=%08X, p1=%08X, p2=%08X\n", wMsg, dwParam1, dwParam2);
		midiOutShortMsg(hmo, dwParam1);
		break;
	}
}


/* INPUT DEVICE */

int deviceInOpen(HMIDIIN *hmi, UINT deviceno)
{
	if (midiInOpen(hmi, deviceno, (DWORD_PTR)MidiInProc, 0, CALLBACK_FUNCTION) != MMSYSERR_NOERROR) {
		return -1;
	}
	return 0;
}

int deviceInClose(HMIDIIN *hmi)
{
	if (midiInClose(*hmi) != MMSYSERR_NOERROR) {
		return -1;
	}
	return 0;
}


/* OUTPUT DEVICE */

int deviceOutOpen(HMIDIOUT *hmo, UINT deviceno)
{
	if (midiOutOpen(hmo, deviceno, 0, 0, CALLBACK_NULL) != MMSYSERR_NOERROR) {
		return -1;
	}
	return 0;
}

int deviceOutClose(HMIDIOUT *hmo)
{
	if (midiOutClose(*hmo) != MMSYSERR_NOERROR) {
		return -1;
	}
	return 0;
}


/* MAIN */

int main(void)
{
	int scan;
	int ret;
	int input_ok = 0;
	int output_ok = 0;

	list_input_devices();
	printf("\n");

	list_output_devices();
	printf("\n");


        
	// ポインタとデバイスIDを渡してMIDI INデバイスを開く
	while (input_ok==0) {
		printf("Enter input device number: ");
		ret = scanf("%d", &scan);
		if (ret==0) {
			while (getchar() != '\n'){}
			continue;
		}
		if (deviceInOpen(&hmi, scan) < 0) {
			printf("MIDI INデバイスオープン失敗\n");
		} else {
			input_ok = 1;
		}
	}
	printf("Input device ready.\n\n");

	// ポインタとデバイスIDを渡してMIDI OUTデバイスを開く
	while (output_ok==0) {
		printf("Enter output device number: ");
		ret = scanf("%d", &scan);
		if (ret==0) {
			while (getchar() != '\n'){}
			continue;
		}
		if (deviceOutOpen(&hmo, scan) < 0) {
			printf("MIDI OUTデバイスオープン失敗\n");
		} else {
			output_ok = 1;
		}
	}
	printf("Output device ready.\n\n");

	printf("--------------------\n");
	printf("Devices ready.\n");
	printf("--------------------\n");

	// 入力開始
	midiInStart(hmi);

	while (1) {
		Sleep(100);
	}

	// 入力停止
	midiInStop(hmi);
	midiInReset(hmi);

	// ポインタを渡してMIDI OUTデバイスを閉じる
	deviceOutClose(&hmo);

	// ポインタを渡してMIDI INデバイスを閉じる
	deviceInClose(&hmi);

	return 0;
}
