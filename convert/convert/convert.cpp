// convert.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "convert.h"
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <new>
#include "capture.h"
#include <d3d9.h>

#define MAX_LOADSTRING 100

CRITICAL_SECTION  m_critial;

IDirect3D9 *m_pDirect3D9 = NULL;
IDirect3DDevice9 *m_pDirect3DDevice = NULL;
IDirect3DSurface9 *m_pDirect3DSurfaceRender = NULL;

RECT m_rtViewport;
RECT desktop;


#define INBUF_SIZE 20480

#define MAX_LOADSTRING 100

UCHAR* CaptureBuffer = NULL;

// Global Variables:
HINSTANCE hInst;                        // current instance
TCHAR szTitle[MAX_LOADSTRING];          // The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];    // the main window class name

// Forward declarations of functions included in this code module:

ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);


bool captured = false;
AVPacket *pkt;

void Cleanup()
{
	EnterCriticalSection(&m_critial);
	if (m_pDirect3DSurfaceRender)
		m_pDirect3DSurfaceRender->Release();
	if (m_pDirect3DDevice)
		m_pDirect3DDevice->Release();
	if (m_pDirect3D9)
		m_pDirect3D9->Release();
	
	LeaveCriticalSection(&m_critial);
}

void AVCodecCleanup()
{
	free(is_keyframe);


	free(CaptureBuffer);

	/*avcodec_close(encodingCodecContext);
	avcodec_close(decodingCodecContext);
	avcodec_free_context(&decodingCodecContext);
	av_frame_free(&iframe);
	av_frame_free(&oframe);
	avcodec_free_context(&encodingCodecContext);*/
	av_frame_free(&inframe);
	av_frame_free(&outframe);
	av_packet_free(&pkt);
	if (cctx) {
		avcodec_free_context(&cctx);
	}
	if (outFmtCtx) {
		avformat_free_context(outFmtCtx);
	}
	
	Cleanup();

	exit(0);
}

int InitD3D(HWND hwnd, unsigned long lWidth, unsigned long lHeight)
{
	HRESULT lRet;
	InitializeCriticalSection(&m_critial);
	Cleanup();

	m_pDirect3D9 = Direct3DCreate9(D3D_SDK_VERSION);
	if (m_pDirect3D9 == NULL)
		return -1;

	D3DPRESENT_PARAMETERS d3dpp;
	ZeroMemory(&d3dpp, sizeof(d3dpp));
	d3dpp.Windowed = TRUE;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;

	GetClientRect(hwnd, &m_rtViewport);

	lRet = m_pDirect3D9->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hwnd,
		D3DCREATE_HARDWARE_VERTEXPROCESSING,
		&d3dpp, &m_pDirect3DDevice);
	if (FAILED(lRet))
		return -1;

	lRet = m_pDirect3DDevice->CreateOffscreenPlainSurface(
		lWidth, lHeight,
		(D3DFORMAT)MAKEFOURCC('Y', 'V', '1', '2'), //FourCC for YUV420P Pixal Color Format
		D3DPOOL_DEFAULT,
		&m_pDirect3DSurfaceRender,
		NULL);


	if (FAILED(lRet))
		return -1;

	return 0;
}



int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Place code here.

    // Initialize global strings
    LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadString(hInstance, IDC_CONVERT, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_CONVERT));

    MSG msg;

    // Main message loop:
	// Main message loop:
	
	ZeroMemory(&msg, sizeof(msg));

	while (msg.message != WM_QUIT) {
		//PeekMessage, not GetMessage
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else {
			driver(msg.hwnd);
			AVCodecCleanup();
		}
	}

	UnregisterClass(szWindowClass, hInstance);
	return 0;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEX wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_CONVERT));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCE(IDC_CONVERT);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassEx(&wcex);

	
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	HWND hWnd;

	hInst = hInstance; // Store instance handle in our global variable

	int pixel_w = 1008, pixel_h = 465;

	hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW, 0, 0, pixel_w, pixel_h, NULL, NULL, hInstance, NULL);

	if (!hWnd)
	{
		return FALSE;
	}

	GetWindowRect(GetDesktopWindow(), &desktop);
	pixel_w = desktop.right - desktop.left;
	pixel_h = desktop.bottom - desktop.top;

	if (InitD3D(hWnd, pixel_w, pixel_h) == E_FAIL) {
		return FALSE;
	}

	/*ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);
*/
	return TRUE;
}
void initmuxing(){
	int err;
	outFmt = av_guess_format("mp4", NULL, NULL);
	if (!outFmt) {
        fprintf(stderr, "Could not find suitable output format\n");
    
    }
	avformat_alloc_output_context2(&outFmtCtx, outFmt, NULL, NULL);
	if (!outFmtCtx) {
        fprintf(stderr, "Could not deduce output format from file extension: using MPEG\n");
    }
	//streams are like video stream or audio stream which is having data and we get frames from streams only
	stream = avformat_new_stream(outFmtCtx, 0);
	if (!stream) {
        fprintf(stderr, "Could not allocate stream\n");
    }
	muxcodec = avcodec_find_encoder(outFmt->video_codec);
	if (!muxcodec)
	{
		std::cout << "can't create codec" << std::endl;
	}
	
	cctx= avcodec_alloc_context3(muxcodec);
	if (!cctx)
	{
		std::cout << "can't create codec context" << std::endl;
	}
	//setting details on stream
	cctx->codec_id = AV_CODEC_ID_H264;
	stream->codecpar->codec_id = outFmt->video_codec;
	stream->codecpar->codec_type = AVMEDIA_TYPE_VIDEO;
	stream->codecpar->width = width;
	stream->codecpar->height =height;
	stream->codecpar->format = AV_PIX_FMT_YUV420P;
	stream->codecpar->bit_rate = 400000;
	if (stream->codecpar->codec_id == AV_CODEC_ID_H264) {
		av_opt_set(cctx->priv_data, "preset", "veryfast", 0);
		av_opt_set(cctx->priv_data, "tune", "zerolatency", 0);
		av_opt_set(cctx->priv_data, "crf", "10", 0);
		av_opt_set(cctx->priv_data, "vprofile", "baseline", 0);
	}
	//stream details copied to codec context
	avcodec_parameters_to_context(cctx, stream->codecpar);
	cctx->time_base.num = 1;
	cctx->time_base.den = 20;
	cctx->framerate.num = 20;
	cctx->framerate.den = 1;
	
	if ((err = avcodec_open2(cctx, muxcodec, NULL)) < 0) {
		std::cout << "Failed to open codec" << err << std::endl;
	}
	av_dump_format(outFmtCtx, 0, "video.mp4", 1); //format context get to know that the data will be mp4

	if (!(outFmt->flags & AVFMT_NOFILE)) {
		if ((err = avio_open(&outFmtCtx->pb, "video.mp4", AVIO_FLAG_WRITE)) < 0) {
			std::cout << "Failed to open file" << err << std::endl;
		}
	}
	
	err=avformat_write_header(outFmtCtx,NULL);//before puting data in format it is neede to write header
	if (err  < 0) {
		std::cout <<"Failed to write header" << err << std::endl;
	}
}
void write_frame(){
	int ret=0;
	if ((ret = avcodec_send_frame(cctx, outframe)) < 0) { //converted frame data send to codec context(cctx)
			std::cout << "Failed to send frame" << ret <<std::endl;
			return;
	}
		ret=0;
	while (ret >= 0)
	{
			    ret =avcodec_receive_packet(cctx, pkt); //from codec context data get stored in pkt
				if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
					continue;
				else if (ret < 0) {
					std::cout<<"Error during encoding\n";
					break;
				}
				if (pkt->size){
					av_packet_rescale_ts(pkt, cctx->time_base, stream->time_base);
					int ret = av_interleaved_write_frame(outFmtCtx, pkt);//writing packet data into outfmtctx(format context which is set for mp4 muxing)
				}
	}
}
void finish(){
	av_write_trailer(outFmtCtx); // at the end of muxing it is needed to write trailor
	if (!(outFmt->flags & AVFMT_NOFILE)) {
		int err = avio_close(outFmtCtx->pb);
		if (err < 0) {
			std::cout << "Failed to close file" << err <<std::endl;
		}
	}
}
void driver(HWND hWnd)
{
	int ret,i;
	RECT rcClient;
	GetClientRect(NULL, &rcClient);
	const HWND hDesktop = GetDesktopWindow();
	GetWindowRect(hDesktop, &desktop);//find width and ht. on the basis of desktop 
	width = desktop.right;
	height = desktop.bottom;

	initmuxing();
	
	pkt = av_packet_alloc();// after encoding save in this
	if (!pkt)
		AVCodecCleanup();
	inframe = av_frame_alloc(); //raw data in rgb is stored in this
	if (!inframe) {
		std::cout<<"Could not allocate video frame\n";
		AVCodecCleanup();
	}
	inframe->format = AV_PIX_FMT_RGB32;
	inframe->width = width;
	inframe->height = height;
	ret = av_frame_get_buffer(inframe, 32);
	if (ret < 0) {
		std::cout<<"Could not allocate the video frame data\n";
		AVCodecCleanup();
	}

	outframe = av_frame_alloc();//after rgb to yuv frame stored in this
	if (!outframe) {
		std::cout<<  "Could not allocate video frame\n";
		AVCodecCleanup();
	}
	outframe->format = AV_PIX_FMT_YUV420P;
	outframe->width = width;
	outframe->height = height;
	ret = av_frame_get_buffer(outframe, 32);
	if (ret < 0) {
		std::cout<< "Could not allocate the video frame data\n";
		AVCodecCleanup();
	}

	if (av_frame_make_writable(outframe) < 0)
		exit(1);

	long CaptureLineSize = NULL;
	ScreenCaptureProcessorGDI *D3D11 = new ScreenCaptureProcessorGDI();
	D3D11->init();
	CaptureBuffer = (UCHAR*)malloc(inframe->height*inframe->linesize[0]);
	rgb_to_yuv_SwsContext = sws_getContext(inframe->width, inframe->height, AV_PIX_FMT_RGB32,outframe->width, outframe->height, AV_PIX_FMT_YUV420P,
		SWS_BICUBIC, NULL, NULL, NULL);//initialize context for conversion
	if (!rgb_to_yuv_SwsContext) {
		AVCodecCleanup();
	}

	char buf[1024];
	int number_of_frames_to_process = 1000;
	is_keyframe = (char *)malloc(number_of_frames_to_process + 10);
	for (i = 0; i < number_of_frames_to_process; i++)
	{
		av_init_packet(pkt);
		fflush(stdout);
		char * srcData = NULL;
		D3D11->GrabImage(CaptureBuffer, CaptureLineSize); // getting image in capturebuffer
		uint8_t * inData[1] = { (uint8_t *)CaptureBuffer }; // RGB have one plane
		int inLinesize[1] = { av_image_get_linesize(AV_PIX_FMT_RGB32, inframe->width, 0) }; // RGB stride
		int scale_rc = sws_scale(rgb_to_yuv_SwsContext, inData, inLinesize, 0, inframe->height, outframe->data,
			outframe->linesize);//converion from rgdb to yuv
		outframe->pts = i;
		write_frame(); 
		
	}
	finish();
	
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {

    case WM_DESTROY:
		Cleanup();
		PostQuitMessage(0);
		return 0;
	case WM_SIZE:
		return 0;
	}
   return DefWindowProc(hWnd, message, wParam, lParam);
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}
