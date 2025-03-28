#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <locale.h>
#include <math.h>
#include <conio.h>
#include <commdlg.h>
#define ID_HelloBut 1
#define AddEnemy(a,b) ObjectInit(NewObject(),a,b,40,40,'e');
typedef struct SPoint{
    float x,y;
} TPoint;
//6.12
HWND g_hWnd;
int killedEnemies = 0; // Счетчик убитых врагов
COLORREF playerColor = RGB(0, 255, 0); // Цвет по умолчанию
COLORREF entityColor = RGB(255,0,0);
COLORREF bulletColor = RGB(0, 255, 0);
COLORREF customColors[16] = {0};
HBITMAP hPlayerBmp = NULL;
HBITMAP hEnemyBmp = NULL;
HBITMAP hBulletBmp = NULL;

TPoint point(float x,float y){
    TPoint pt;
    pt.x = x;
    pt.y = y;
    return pt;
}
TPoint mousePos = {0, 0};
BOOL Pause = FALSE;
BOOL needNewGame = FALSE;

TPoint cam;

typedef struct SObject{
    TPoint pos;
    TPoint size;
    COLORREF brush;
    TPoint speed;
    char oType;
    float range,vecSpeed;
    BOOL isDel;
} TObject, *PObject;


BOOL ObjectCollision(TObject o1,TObject o2){
    return ((o1.pos.x +o1.size.x)>o2.pos.x) && (o1.pos.x <(o2.pos.x +o2.size.x))&&
        ((o1.pos.y + o1.size.y)>o2.pos.y)&& (o1.pos.y <(o2.pos.y+o2.size.y));
}
RECT rct;
TObject player;

PObject mas = NULL;
int masCnt = 0;

void ObjectInit(TObject *obj, float xPos, float yPos, float width, float height,char objType){
    obj->pos = (point(xPos,yPos));
    obj-> size = point(width,height);
    obj->brush = RGB(0,255,0);
    obj->speed = point(0,0);
    obj->oType = objType;
    if (objType == 'e') obj->brush = RGB(255,0,0);
    obj->isDel = FALSE;
}

void ObjectShow(TObject obj, HDC dc){
    SelectObject(dc, GetStockObject(DC_PEN));
    SetDCPenColor(dc,RGB(0,0,0));
    SelectObject(dc,GetStockObject(DC_BRUSH));
    SetDCBrushColor(dc,obj.brush);
    BOOL (*shape)(HDC,int,int,int,int);
    if (obj.oType == 'p') {
        shape = obj.oType == 'p' ? Rectangle: Rectangle;
        shape(dc,(int)(obj.pos.x-cam.x),(int)(obj.pos.y-cam.y),(int)(obj.pos.x+obj.size.x-cam.x),(int)(obj.pos.y+obj.size.y-cam.y));


    } else if (obj.oType == 'e') {
        
        Ellipse(dc, 
            (int)(obj.pos.x - cam.x), 
            (int)(obj.pos.y - cam.y), 
            (int)(obj.pos.x + obj.size.x - cam.x), 
            (int)(obj.pos.y + obj.size.y - cam.y));
        
    } else if (obj.oType == '1') {
        RoundRect(dc, 
                  (int)(obj.pos.x - cam.x), 
                  (int)(obj.pos.y - cam.y), 
                  (int)(obj.pos.x + obj.size.x - cam.x), 
                  (int)(obj.pos.y + obj.size.y - cam.y),5,5);
    }
}

void ObjectSetDestPoint(TObject *obj,float xPos,float yPos,float vecSpeed){
    TPoint xyLen = point(xPos-obj->pos.x,yPos-obj->pos.y);
    float dxy = sqrt(xyLen.x * xyLen.x+ xyLen.y * xyLen.y);
    obj->speed.x = xyLen.x/dxy *vecSpeed;
    obj->speed.y = xyLen.y/dxy *vecSpeed;
    obj->vecSpeed = vecSpeed;
}

void ObjectMove(TObject *obj){

    if (obj->oType == 'e'){
        if (rand() %40 == 1){
            static float enemySpeed = 6;
            ObjectSetDestPoint(obj,player.pos.x,player.pos.y,enemySpeed);
        }
        if (ObjectCollision(*obj,player))
        {
            needNewGame = TRUE;
            if (needNewGame == TRUE) {
                
                KillTimer(g_hWnd, 1);
                int MsgId = MessageBox(NULL, "Вас убили\nНачать новую игру?", "Поражения", MB_YESNO | MB_ICONHAND);
                
                    switch (MsgId) {
                        case IDYES:
                            needNewGame = FALSE;
                            WinInit();
                            SetTimer(g_hWnd, 1, 16, NULL); // ~60 FPS
                            break;
                        case IDNO:
                            exit(-1);
                            break;
                        default:
                            break;
                    }
            }
            return;
        }
    }
        
    obj->pos.x += obj->speed.x;
    obj->pos.y += obj->speed.y;

    if (obj->oType == '1'){
        obj->range -= obj->vecSpeed;
        if (obj->range<0)
            obj->isDel = TRUE;
        for (int i = 0; i<masCnt;i++)
            if ((mas[i].oType == 'e') && ObjectCollision(*obj,mas[i])){
                if (obj->oType=='p')
                    return NULL;
                else{
                    mas[i].isDel = TRUE;
                    obj->isDel = TRUE;
                    killedEnemies++;
                }   
            }
    }
}

PObject NewObject(){
    masCnt++;
    mas = realloc(mas,sizeof(*mas)*masCnt);
    return mas + masCnt -1;
}

void GenNewEnemy(){
    static int rad = 300;
    int pos1 = (rand()%2 == 0 ? -rad : rad);
    int pos2 = (rand() % (rad*2)) - rad;
    int k = rand() % 30;
    if (k==1)
        AddEnemy(player.pos.x + pos1, player.pos.y  +pos2);
    if (k==2)
        AddEnemy(player.pos.x + pos2, player.pos.y + pos1);
}

void DelObjects(){
    int i = 0;
    while (i <masCnt){
        if (mas[i].isDel){
            masCnt--;
            mas[i] = mas[masCnt];
            mas = realloc(mas,sizeof(*mas) *masCnt);
        }
        else
            i++;
    }
}

void AddBullet(float xPos, float yPos, float x, float y){
    PObject obj = NewObject();
    ObjectInit(obj,xPos,yPos,10,10,'1');
    ObjectSetDestPoint(obj,x,y,20);
    obj->range = 300;
}

void PlayerContol(){
    static int PlayerSpeed = 10;
    player.speed.x = 0;
    player.speed.y = 0;
    if (GetKeyState('W')<0) player.speed.y = -PlayerSpeed;
    if (GetKeyState('S')<0) player.speed.y = PlayerSpeed;
    if (GetKeyState('A')<0) player.speed.x = -PlayerSpeed;
    if (GetKeyState('D')<0) player.speed.x = PlayerSpeed;
    if ((player.speed.x != 0 )&& (player.speed.y != 0))
        player.speed = point(player.speed.x *0.7,player.speed.y*0.7);
    }


void WinMove(){
    
    PlayerContol();
    ObjectMove(&player);
    SetCameraFocus(player);
    for (int i = 0;i<masCnt;i++){
        ObjectMove(mas+i);
    }
    GenNewEnemy();
    DelObjects();
}

void SetCameraFocus(TObject obj){
    cam.x = obj.pos.x - rct.right/2;
    cam.y = obj.pos.y - rct.bottom/2;
}

void WinInit(){
    
    //hEnemyBmp = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_ENEMY));
    //hBulletBmp = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_BULLET));
    killedEnemies = 0; // Сбрасываем счетчик
    needNewGame = FALSE;
    masCnt = 0;
    mas = realloc(mas,0);

    ObjectInit(&player,100,100,40,40,'p');

}
void WinShow(HDC dc){
    HDC memDC = CreateCompatibleDC(dc);
    HBITMAP memBM = CreateCompatibleBitmap(dc,rct.right-rct.left,rct.bottom-rct.top);
    SelectObject(memDC,memBM);
        SelectObject(memDC, GetStockObject(DC_PEN));
        SetDCPenColor(memDC,RGB(255,255,255));
        SelectObject(memDC, GetStockObject(DC_BRUSH));
        SetDCBrushColor(memDC,RGB(200,200,200));
        HICON hIcon = LoadIcon(GetModuleHandle(NULL),"Ex4_Icon");
        static int rectSize = 200;
        int dx = (int)(cam.x)%rectSize;
        int dy = (int)(cam.y)%rectSize;

        for (int i = -1; i<(rct.right/rectSize)+2;i++)
            for (int j = -1;j<(rct.bottom/rectSize)+2;j++)
                Rectangle(memDC,-dx+(i*rectSize),-dy+(j*rectSize),-dx+((i+1)*rectSize),-dy+((j+1)*rectSize));

        ObjectShow(player,memDC);
        for (int i = 0; i<masCnt;i++){
            if (mas[i].oType == 'e') mas[i].brush = entityColor;
            if (mas[i].oType == '1') mas[i].brush = bulletColor;
            ObjectShow(mas[i],memDC);
        }

        char scoreText[32];
        sprintf(scoreText, "Убито: %d", killedEnemies);
        SetTextColor(memDC, RGB(0, 0, 0));
        SetBkMode(memDC, TRANSPARENT);
        TextOutA(memDC, rct.right - 150, 10, scoreText, strlen(scoreText));
    BitBlt(dc,0,0,rct.right-rct.left,rct.bottom-rct.top,memDC,0,0,SRCCOPY);
    DeleteDC(memDC);
    DeleteObject(memBM);
}
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    HDC hdc;
    PAINTSTRUCT ps;

    switch (msg) {
        case WM_CREATE:
            player.brush = playerColor;
            g_hWnd = hwnd;
            SetTimer(hwnd, 1, 16, NULL); // ~60 FPS
            WinInit();
            return 0;

        case WM_TIMER:
            if (getch() == '27') 
            {
                KillTimer(hwnd,1);
                while(TRUE){
                    if (getch() == '27') SetTimer(hwnd, 1, 16, NULL);
                }
            }
            WinMove();
            InvalidateRect(hwnd, NULL, TRUE);
            return 0;

        case WM_PAINT:
            hdc = BeginPaint(hwnd, &ps);
            GetClientRect(hwnd, &rct); // Получаем размеры клиентской области
            WinShow(hdc);
            EndPaint(hwnd, &ps);
            return 0;
        case WM_LBUTTONDOWN:
            // Получаем координаты мыши из lParam
            mousePos.x = (float)LOWORD(lParam);
            mousePos.y = (float)HIWORD(lParam);
            AddBullet(player.pos.x + player.size.x/2,player.pos.y +player.size.y/2,mousePos.x + cam.x,mousePos.y+cam.y);
            return 0;
        case WM_KEYDOWN: // Обработка нажатия клавиш
            switch (wParam) {
                case VK_ESCAPE:
                    if (Pause){
                        SetTimer(hwnd, 1, 16, NULL);
                        Pause = FALSE;
                    }
                    else {
                        KillTimer(hwnd,1);
                        Pause =  TRUE;
                    }
                    return 0;
                case VK_F2:{
                    KillTimer(hwnd,1);
                    CHOOSECOLOR cc = {0};
                    cc.lStructSize = sizeof(CHOOSECOLOR);
                    cc.hwndOwner = hwnd; // Нет родительского окна на этом этапе
                    cc.rgbResult = playerColor; // Цвет по умолчанию
                    cc.lpCustColors = customColors; // Массив пользовательских цветов
                    cc.Flags = CC_RGBINIT | CC_FULLOPEN; // Инициализация цветом + полное окно
                    if (ChooseColor(&cc)) {
                        playerColor = cc.rgbResult;
                        player.brush = cc.rgbResult; // Сохраняем выбранный цвет
                        SetTimer(hwnd, 1, 16, NULL);
                    } else {
                        // Если пользователь отменил выбор, завершаем программу
                        SetTimer(hwnd, 1, 16, NULL);
                        return 0;
                    }
                    return 0;
                }
                case VK_F3:{
                    KillTimer(hwnd,1);
                    CHOOSECOLOR cc = {0};
                    cc.lStructSize = sizeof(CHOOSECOLOR);
                    cc.hwndOwner = hwnd; // Нет родительского окна на этом этапе
                    cc.rgbResult = entityColor; // Цвет по умолчанию
                    cc.lpCustColors = customColors; // Массив пользовательских цветов
                    cc.Flags = CC_RGBINIT | CC_FULLOPEN; // Инициализация цветом + полное окно
                    if (ChooseColor(&cc)) {
                        printf("aaaa");
                        entityColor = cc.rgbResult;
                        for (int i = 0; i<masCnt;i++)
                        {
                            if (mas[i].oType == 'e') mas[i].brush = entityColor;
                        }
                        SetTimer(hwnd, 1, 16, NULL);
                    } else {
                        // Если пользователь отменил выбор, завершаем программу
                        SetTimer(hwnd, 1, 16, NULL);
                        return 0;
                    }
                    return 0;
                }
                case VK_F4:{
                    KillTimer(hwnd,1);
                    CHOOSECOLOR cc = {0};
                    cc.lStructSize = sizeof(CHOOSECOLOR);
                    cc.hwndOwner = hwnd; // Нет родительского окна на этом этапе
                    cc.rgbResult = bulletColor; // Цвет по умолчанию
                    cc.lpCustColors = customColors; // Массив пользовательских цветов
                    cc.Flags = CC_RGBINIT | CC_FULLOPEN; // Инициализация цветом + полное окно
                    if (ChooseColor(&cc)) {
                        bulletColor = cc.rgbResult;
                        for (int i = 0; i<masCnt;i++)
                        {
                            if (mas[i].oType == '1') mas[i].brush = bulletColor;
                        }
                        SetTimer(hwnd, 1, 16, NULL);
                    } else {
                        // Если пользователь отменил выбор, завершаем программу
                        SetTimer(hwnd, 1, 16, NULL);
                        return 0;
                    }
                    return 0;
                }
                }
            return 0;
        case WM_DESTROY:
            KillTimer(hwnd, 1);
            free(mas); // Освобождаем память
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInst,HINSTANCE hpi, LPSTR cmdline,int ss) {
	setlocale(LC_ALL,"");
/* создаем и регистрируем класс главного окна */
	
    WNDCLASS wc; 
    wc.style=0; /* стиль класса окна, 0 - по умолчанию. CS_DBLCLKS — окно обрабатывает двойные клики мыши. */
    wc.lpfnWndProc=WndProc; /* Указывает функцию-обработчик сообщений для окон этого класса. */
    wc.cbClsExtra=wc.cbWndExtra=0; /* Резервирует дополнительную память для класса и каждого окна. */
    wc.hInstance=hInst; /* Связывает класс окна с текущим экземпляром приложения.
    hInst — дескриптор экземпляра приложения, переданный в WinMain.
    Это позволяет системе идентифицировать, какому приложению принадлежит класс окна.	*/
    wc.hIcon=NULL; /* Задает иконку окна.
    wc.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_MY_ICON));	*/
    wc.hCursor=LoadCursorA(NULL,IDC_CROSS); /* Задает курсор мыши для окна.
    wc.hCursor = LoadCursor(hInst, MAKEINTRESOURCE(IDC_MY_CURSOR));	*/
    wc.hbrBackground=RGB(0,0,0); /* Задает цвет фона окна.
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1); - системный цвет окон
    */
    wc.lpszMenuName="Ex4_Menu"; /* Указывает меню по умолчанию для окон этого класса.
    wc.lpszMenuName = MAKEINTRESOURCE(IDR_MY_MENU); // Имя ресурса меню.	*/
    wc.lpszClassName="Example MainWnd Class"; /* Уникальное имя класса окна. */
    if (!RegisterClass(&wc)) /* Регистрирует класс окна в системе.
    Если регистрация не удалась (например, имя класса уже занято), функция вернет 0, и приложение завершится.	*/
        return FALSE;
 /* создаем главное окно и отображаем его */
	HWND hMainWnd = CreateWindow(
	"Example MainWnd Class", /* имя класса */
	"Shouter", /* имя окна (заголовок) */
	WS_OVERLAPPEDWINDOW, /* стиль (поведение) окна */
	10, /* горизонтальная позиция окна на экране */
	10, /* вертикальная позиция окна на экране */
	640, /* ширина окна */
	480, /* высота окна */
	NULL, /* дескриптор родительского окна */
	NULL, /* дескриптор меню */
	hInst, /* дескриптор экземпляра программы */
	NULL); /* указатель на какую-нибудь ерунду */
	if (!hMainWnd) /* Проверяет, был ли дескриптор окна (HWND) успешно создан. */
		return FALSE;
	HDC dc = GetDC(hMainWnd);
	ShowWindow(hMainWnd,ss); /* Отображение окна. Дескриптор созданного окна, начальное состояние окна */
	UpdateWindow(hMainWnd); /* перерисовать клиентскую область. WM_PAINT. Без этого окно может отображаться
	с пустой клиентской областью до первого изменения размера или перемещения. */

	MSG msg; /* Объявление структуры сообщения */
	/* цикл обработки событий */
	while (GetMessage(&msg,NULL,0,0)) { /* Извлекает сообщение из очереди сообщений приложения.
&msg - указатель на структуру MSG, куда будет записано сообщение.
NULL — получать сообщения для всех окон приложения. 
0, 0 — фильтры сообщений (здесь отключены).
Если очередь сообщений пуста, GetMessage блокирует выполнение, пока не придет новое сообщение.*/
		TranslateMessage(&msg); /* Преобразует сообщения от клавиатуры в символы.
Пользователь нажал клавишу A ? WM_KEYDOWN с кодом клавиши.
TranslateMessage генерирует WM_CHAR с символом A.		*/
		DispatchMessage(&msg); /* Передает сообщение в оконную процедуру (WndProc),
		связанную с окном-получателем (msg.hwnd). 
		Система вызывает функцию WndProc, указанную при регистрации класса окна.
		WndProc обрабатывает сообщение (например, закрытие окна, клик мыши).*/
	} 
	
	return msg.wParam; /*  Возвращает код завершения приложения. */
}

