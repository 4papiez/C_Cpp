/** Author: Mateusz Papie¿ */


#include <math.h>
#include <iostream>
#include <conio.h>
#include "graphics.h"


void draw_menu(void)
{
 int x;
 for (x=0;x<256;x++) { setcolor(COLOR(x,255,0)); line(x,0,x,30);}
 for (x=0;x<256;x++) { setcolor(COLOR(255,255-x,0)); line(255+x,0,255+x,30);}
 for (x=0;x<256;x++) { setcolor(COLOR(255,0,x)); line(510+x,0,510+x,30);}
 for (x=0;x<256;x++) { setcolor(COLOR(0,255,255-x)); line(255-x,30,255-x,60);}
 for (x=0;x<256;x++) { setcolor(COLOR(0,x,255)); line(510-x,30,510-x,60); }
 for (x=0;x<256;x++) { setcolor(COLOR(255-x,0,255)); line(765-x,30,765-x,60);}

 setcolor(COLOR(255,255,255));
 outtextxy(5,600,"f - wybór koloru rysowania");
 outtextxy(5,615,"b - wybór koloru wype³niania");
 outtextxy(5,630,"l - rysowanie linii");

 outtextxy(200,600,"r - rysowanie prostok¹ta");
 outtextxy(200,615,"a - rysowanie wype³nionego prostok¹ta");
 outtextxy(200,630,"c - rysowanie okrêgu");

 outtextxy(470,600,"w - zapis do pliku");
 outtextxy(470,615,"o - odczyt z pliku");
 outtextxy(470,630,"esc - wyjœcie");

 outtextxy(650,615,"Aktualne:");

 setcolor(COLOR(255,255,255));
 rectangle(1,62,798,598);
}

int main( )
{
    int x,y;
    int x2,y2;
    int kolor;
    initwindow(800, 700, "First Sample");
    draw_menu();
    int flaga,miniflaga;
    void far *img = malloc(imagesize(0,0,800,700));
    setlinestyle(SOLID_LINE,2, 2);
    //setviewport(1, 62, 798, 598, true);
    do{
        flaga=getch();
        miniflaga=0;
     //------------------------------------------------------------------
            while(!kbhit()&&flaga==102){
                setcolor(COLOR(255,255,255));
                outtextxy(725,615,"F");
                if( ismouseclick(WM_LBUTTONDOWN)){
                    getmouseclick(WM_LBUTTONDOWN, x, y);
                    kolor = getpixel(x, y);
                    setfillstyle( SOLID_FILL,kolor);
                    bar(766,0,800,30);
                    
                    
                }    
            }
     //------------------------------------------------------------------
            while(!kbhit()&&flaga==98){
                setcolor(COLOR(255,255,255));
                outtextxy(725,615,"B");
                if( ismouseclick(WM_LBUTTONDOWN)){
                    getmouseclick(WM_LBUTTONDOWN, x, y);
                    kolor = getpixel(x, y);
                    setfillstyle( SOLID_FILL,kolor);
                    bar(766,30,800,60);
                    //std::cout<<kolor<<std::endl;
                }    
            }
     //------------------------------------------------------------------
            while(!kbhit()&&flaga==108){
                setcolor(COLOR(255,255,255));
                outtextxy(725,615,"L");
                setcolor(getpixel(780,15));
                if( ismouseclick(WM_LBUTTONDOWN)&&miniflaga==0){
                    getmouseclick(WM_LBUTTONDOWN, x, y);
                    miniflaga = 1;
                    getimage(0,0,800,700,img);
                }
    
                while(miniflaga==1&&!kbhit()){
                    if(ismouseclick(WM_LBUTTONDOWN)&&miniflaga==1){
                        getmouseclick(WM_LBUTTONDOWN, x2, y2);
                        miniflaga = 2;
                    }
                    else if (ismouseclick (WM_MOUSEMOVE)){
                        putimage(0,0,img,COPY_PUT);
                        line(x,y,mousex(),mousey());
                        delay(1);
                    }
                }
                        
                if(miniflaga == 2){
                    line(x,y,x2,y2);
                    miniflaga =0;

                }
        }
     //------------------------------------------------------------------
            while(!kbhit()&&flaga==114){
                setcolor(COLOR(255,255,255));
                outtextxy(725,615,"R");
                setcolor(getpixel(780,15));
                if( ismouseclick(WM_LBUTTONDOWN)&&miniflaga==0){
                    getmouseclick(WM_LBUTTONDOWN, x, y);
                    miniflaga = 1;
                    getimage(0,0,800,700,img);
                }
                while(miniflaga==1&&!kbhit()){
                    if(ismouseclick(WM_LBUTTONDOWN)&&miniflaga==1){
                        getmouseclick(WM_LBUTTONDOWN, x2, y2);
                        miniflaga = 2;
                    }
                    else if (ismouseclick (WM_MOUSEMOVE)){
                        putimage(0,0,img,COPY_PUT);
                        rectangle(x,y,mousex(),mousey());
                        delay(1);
                    }
                }
                        
                if(miniflaga == 2){
                    rectangle(x,y,x2,y2);
                    miniflaga =0;

                }
            }
     //------------------------------------------------------------------
             while(!kbhit()&&flaga==97){
                setcolor(COLOR(255,255,255));
                outtextxy(725,615,"A");
                setcolor(getpixel(780,15));
                setfillstyle(SOLID_FILL,getpixel(780,45));
               if( ismouseclick(WM_LBUTTONDOWN)&&miniflaga==0){
                    getmouseclick(WM_LBUTTONDOWN, x, y);
                    miniflaga = 1;
                    getimage(0,0,800,700,img);
                }
                while(miniflaga==1&&!kbhit()){
                    if(ismouseclick(WM_LBUTTONDOWN)&&miniflaga==1){
                        getmouseclick(WM_LBUTTONDOWN, x2, y2);
                        miniflaga = 2;
                    }
                    else if (ismouseclick (WM_MOUSEMOVE)){
                        putimage(0,0,img,COPY_PUT);
                        bar(x,y,mousex(),mousey());
                        delay(1);
                    }
                }
                        
                if(miniflaga == 2){
                    bar(x,y,x2,y2);
                    miniflaga =0;

                }
            }
     //------------------------------------------------------------------
            while(!kbhit()&&flaga==99){
                setcolor(COLOR(255,255,255));
                outtextxy(725,615,"C");
                setcolor(getpixel(780,15));
                if( ismouseclick(WM_LBUTTONDOWN)&&miniflaga==0){
                    getmouseclick(WM_LBUTTONDOWN, x, y);
                    miniflaga = 1;
                    getimage(0,0,800,700,img);
                }
                while(miniflaga==1&&!kbhit()){
                    if(ismouseclick(WM_LBUTTONDOWN)&&miniflaga==1){
                        getmouseclick(WM_LBUTTONDOWN, x2, y2);
                        miniflaga = 2;
                    }
                    else if (ismouseclick (WM_MOUSEMOVE)){
                        putimage(0,0,img,COPY_PUT);
                        circle(x,y,sqrt((x-mousex())*(x-mousex())+(y-mousey())*(y-mousey())));
                        delay(1);
                    }
                }
                        
                if(miniflaga == 2){
                    circle(x,y,sqrt((x-x2)*(x-x2)+(y-y2)*(y-y2)));
                    miniflaga =0;

                }
                
                
            }
     //------------------------------------------------------------------
            if(!kbhit()&&flaga==119){
                writeimagefile();
            }
     //------------------------------------------------------------------
            if(!kbhit()&&flaga==111){
                readimagefile(NULL,0,0,800,700);
            }
             
            
            
            
    }while(flaga!=27);
    
    
    //_getch();
    return( 0 );
}
