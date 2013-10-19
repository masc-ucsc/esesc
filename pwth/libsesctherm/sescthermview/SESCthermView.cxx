
#include "SESCthermView.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

double chip_width=3.0;
double chip_height=3.0;
double chip_thickness=1.0;
double heat_spreader_width=7.0;
double heat_spreader_height=7.0;
double heat_spreader_thickness=1.0;
double heat_sink_width=9.0;
double heat_sink_height=9.0;
double heat_sink_thickness=1.0;
double heat_sink_fins_thickness=5.0;
int heat_spreader_fins=9;

void SESCthermView::genTextureQuad(Quad& quad, 
                                   unsigned char* centercolor, 
                                   unsigned char* leftcolor, 
                                   unsigned char* rightcolor, 
                                   unsigned char* topcolor, 
                                   unsigned char* bottomcolor, 
                                   unsigned char* downcolor, 
                                   unsigned char* upcolor){

    //allocate space
    //64X64 pixels
    double width=64;
    double height=64;
    unsigned char* data=new unsigned char[(int)(width*height*3)]; //RGB*numpixels

    int dataitor=0;
    //create the texture for the x-y face (down face)
    for (double y=0;(int)(int)y<(int)height;y++) {
        for (double x=0;(int)x<(int)width;x++) {
            /*
            cout << "colors:" << centercolor[0] << leftcolor[0] << rightcolor[0] << topcolor[0] << bottomcolor[0] << endl;
            cout << "centercolor:" << centercolor[0]*(1.0- sqrt(pow(y-height/2,2)+pow(x-width/2,2))/max(width,height)) << endl;
 cout << "leftcolor:" << leftcolor[0]*(1.0- sqrt(pow(y-height/2,2)+pow(x-0,2))/width)<< endl;
  cout << "rightcolor:" <<  rightcolor[0]*(1.0- sqrt(pow(y-height/2,2)+pow(x-width,2))/width)<< endl;
   cout << "topcolor:" << topcolor[0]*(1.0- sqrt(pow(y-height,2)+pow(x-width/2,2))/height)<< endl;
      cout << "bottomcolor:" <<  bottomcolor[0]*(1.0- sqrt(pow(y-0,2)+pow(x-width/2,2))/height) << endl;
*/
            double temp0=0;
            double temp1=0;
            double temp2=0;
            double temp3=0;
            double temp4=0;




            temp0=1-sqrt(pow(y-(height/2),2)+pow(x-(width/2),2))/sqrt(pow(height/2,2)+pow(width/2,2));



            /*   
                 temp1=1-sqrt(pow(y-(height/2),2)+pow(x-0,2))/sqrt(pow(height/2,2)+pow(width,2));
           

                 temp2=1-sqrt(pow(y-(height/2),2)+pow(x-width,2))/sqrt(pow(height/2,2)+pow(width,2));


                 temp3=1-sqrt(pow(y-height,2)+pow(x-(width/2),2))/sqrt(pow(height,2)+pow(width/2,2));
                 

                 temp4=1-sqrt(pow(y-0,2)+pow(x-(width/2),2))/sqrt(pow(height,2)+pow(width/2,2));

                 */
            //   while(1){

            //    cout << "X" << endl;
            //    cin >> x;
            //        cout << "Y"<<endl;
            //        cin >> y;
            if (x>width/2)
                temp1=0;
            else
                temp1=1-x/(width/2);

            if ((width-x)>width/2)
                temp2=0;

            else
                temp2=1-(width-x)/(width/2);

            if (height-y>height/2)
                temp3=0;
            else

                temp3=1-(height-y)/(height/2);

            if (y>height/2)
                temp4=0;
            else
                temp4=1-y/(height/2);
//cout << temp0 << " " << temp1 << " " << temp2 << " " << temp3 << " "<<temp4<<endl;

            double gamma=1/(temp0+temp1+temp2+temp3+temp4);
            temp0*=gamma;
            temp1*=gamma;
            temp2*=gamma;
            temp3*=gamma;
            temp4*=gamma;
            if ((centercolor[0]*(temp0)+
                 leftcolor[0]*(temp1)+
                 rightcolor[0]*(temp2)+
                 topcolor[0]*(temp3)+
                 bottomcolor[0]*(temp4))>255)
                data[dataitor]=255;
            else
                data[dataitor]=( unsigned char)(centercolor[0]*(temp0)+
                                                leftcolor[0]*(temp1)+
                                                rightcolor[0]*(temp2)+
                                                topcolor[0]*(temp3)+
                                                bottomcolor[0]*(temp4));
            if ((centercolor[1]*(temp0)+
                 leftcolor[1]*(temp1)+
                 rightcolor[1]*(temp2)+
                 topcolor[1]*(temp3)+
                 bottomcolor[1]*(temp4))>255)
                data[dataitor+1]=255;
            else
                data[dataitor+1]=( unsigned char)(centercolor[1]*(temp0)+
                                                  leftcolor[1]*(temp1)+
                                                  rightcolor[1]*(temp2)+
                                                  topcolor[1]*(temp3)+
                                                  bottomcolor[1]*(temp4));
            if ((centercolor[2]*(temp0)+
                 leftcolor[2]*(temp1)+
                 rightcolor[2]*(temp2)+
                 topcolor[2]*(temp3)+
                 bottomcolor[2]*(temp4))>255)
                data[dataitor+1]=255;
            else
                data[dataitor+2]=( unsigned char)(centercolor[2]*(temp0)+
                                                  leftcolor[2]*(temp1)+
                                                  rightcolor[2]*(temp2)+
                                                  topcolor[2]*(temp3)+
                                                  bottomcolor[2]*(temp4));

            //   cout << (unsigned int)data[dataitor]<<","<<(unsigned int)data[dataitor+1]<<","<< (unsigned int)data[dataitor+2]<< endl;
            //    }
            // }

            dataitor+=3;
        }





    }

    quad.down_data=data;






}


SESCthermView::SESCthermView(int x,int y,int w,int h,const char *l)
: Fl_Gl_Window(x,y,w,h,l){
    size=10.0;
    xshift=0.0;
    yshift=0.0;

    /* The quad definition. These are the vertices of a unit cube
     * centered on the origin.*/
    //Allocate 10 such quads
    for (int i=0;i<10;i++) {

        Quad newquad;
        newquad.width=1;
        newquad.height=1;
        newquad.depth=1;

        newquad.color0[0]=0.3;
        newquad.color0[1]=0.2;
        newquad.color0[2]=0.55;
        newquad.color0[3]=0.80;
        //back side: bottomleft
        newquad.back_bottom_left[0] = -1*newquad.width/2; 
        newquad.back_bottom_left[1] = -1*newquad.height/2; 
        newquad.back_bottom_left[2] = -1*newquad.depth/2; 
        //back side: bottomright
        newquad.back_bottom_right[0] =  newquad.width/2; 
        newquad.back_bottom_right[1] = -1*newquad.height/2; 
        newquad.back_bottom_right[2] = -1*newquad.depth/2; 
        //back side: top right
        newquad.back_top_right[0] =  newquad.width/2; 
        newquad.back_top_right[1] =  newquad.height/2; 
        newquad.back_top_right[2] = -1*newquad.depth/2; 
        //back side: top left
        newquad.back_top_left[0] = -1*newquad.width/2; 
        newquad.back_top_left[1] =  newquad.height/2; 
        newquad.back_top_left[2] = -1*newquad.depth/2; 
        //front side: bottomleft
        newquad.front_bottom_left[0] = -1*newquad.width/2; 
        newquad.front_bottom_left[1] = -1*newquad.height/2; 
        newquad.front_bottom_left[2] =  newquad.depth/2; 
        //front side: bottomright
        newquad.front_bottom_right[0] = newquad.width/2; 
        newquad.front_bottom_right[1] = -1*newquad.height/2; 
        newquad.front_bottom_right[2] =  newquad.depth/2; 
        //front side: top right
        newquad.front_top_right[0] =  newquad.width/2; 
        newquad.front_top_right[1] =  newquad.height/2; 
        newquad.front_top_right[2] =  newquad.depth/2; 
        //front side: top left
        newquad.front_top_left[0] = -1*newquad.width/2; 
        newquad.front_top_left[1] =  newquad.height/2; 
        newquad.front_top_left[2] =  newquad.depth/2; 

        unsigned char colorcenter[3]={0,0,100};
        unsigned char colorleft[3]={100,0,0};
        unsigned char colorright[3]={100,0,0};
        unsigned char colortop[3]={0,0,255};
        unsigned char colorbottom[3]={255,0,};
        unsigned char colordown[3]={0,0,0};
        unsigned char colorup[3]={0,0,0};
        genTextureQuad(newquad,colorcenter, 
                       colorleft, 
                       colorright, 
                       colortop, 
                       colorbottom, 
                       colordown, 
                       colorup);
        //front side: top left
        chip_quads_.push_back(newquad);
    }

    for (int i=0;i<50;i++) {

        Quad newquad;
        newquad.width=1;
        newquad.height=1;
        newquad.depth=1;
        newquad.color0[0]=0.3;
        newquad.color0[1]=0.2;
        newquad.color0[3]=0.55;
        newquad.color0[2]=0.80;
        //back side: bottomleft
        newquad.back_bottom_left[0] = -1*newquad.width/2; 
        newquad.back_bottom_left[1] = -1*newquad.height/2; 
        newquad.back_bottom_left[2] = -1*newquad.depth/2; 
        newquad.back_bottom_right[0] =  newquad.width/2; 
        newquad.back_bottom_right[1] = -1*newquad.height/2; 
        newquad.back_bottom_right[2] = -1*newquad.depth/2; 
        //back side: bottomright
        newquad.back_top_right[0] =  newquad.width/2; 
        newquad.back_top_right[1] =  newquad.height/2; 
        newquad.back_top_right[2] = -1*newquad.depth/2; 
        //back side: top right
        newquad.back_top_left[0] = -1*newquad.width/2; 
        newquad.back_top_left[1] =  newquad.height/2; 
        newquad.back_top_left[2] = -1*newquad.depth/2; 
        //back side: top left
        newquad.front_bottom_left[0] = -1*newquad.width/2; 
        newquad.front_bottom_left[1] = -1*newquad.height/2; 
        newquad.front_bottom_left[2] =  newquad.depth/2; 
        //front side: bottomleft
        newquad.front_bottom_right[0] = newquad.width/2; 
        newquad.front_bottom_right[1] = -1*newquad.height/2; 
        newquad.front_bottom_right[2] =  newquad.depth/2; 
        //front side: bottomright
        newquad.front_top_right[0] =  newquad.width/2; 
        newquad.front_top_right[1] =  newquad.height/2; 
        newquad.front_top_right[2] =  newquad.depth/2; 
        //front side: top right
        newquad.front_top_left[0] = -1*newquad.width/2; 
        newquad.front_top_left[1] =  newquad.height/2; 
        newquad.front_top_left[2] =  newquad.depth/2; 
        //front side: top left

        heat_spreader_quads_.push_back(newquad);
    }
    for (int i=0;i<100;i++) {

        Quad newquad;
        newquad.width=1;
        newquad.height=1;
        newquad.depth=1;
        newquad.color0[3]=0.3;
        newquad.color0[0]=0.2;
        newquad.color0[2]=0.55;
        newquad.color0[1]=0.80;
        //back side: bottomleft
        newquad.back_bottom_left[0] = -1*newquad.width/2; 
        newquad.back_bottom_left[1] = -1*newquad.height/2; 
        newquad.back_bottom_left[2] = -1*newquad.depth/2; 
        newquad.back_bottom_right[0] =  newquad.width/2; 
        newquad.back_bottom_right[1] = -1*newquad.height/2; 
        newquad.back_bottom_right[2] = -1*newquad.depth/2; 
        //back side: bottomright
        newquad.back_top_right[0] =  newquad.width/2; 
        newquad.back_top_right[1] =  newquad.height/2; 
        newquad.back_top_right[2] = -1*newquad.depth/2; 
        //back side: top right
        newquad.back_top_left[0] = -1*newquad.width/2; 
        newquad.back_top_left[1] =  newquad.height/2; 
        newquad.back_top_left[2] = -1*newquad.depth/2; 
        //back side: top left
        newquad.front_bottom_left[0] = -1*newquad.width/2; 
        newquad.front_bottom_left[1] = -1*newquad.height/2; 
        newquad.front_bottom_left[2] =  newquad.depth/2; 
        //front side: bottomleft
        newquad.front_bottom_right[0] = newquad.width/2; 
        newquad.front_bottom_right[1] = -1*newquad.height/2; 
        newquad.front_bottom_right[2] =  newquad.depth/2; 
        //front side: bottomright
        newquad.front_top_right[0] =  newquad.width/2; 
        newquad.front_top_right[1] =  newquad.height/2; 
        newquad.front_top_right[2] =  newquad.depth/2; 
        //front side: top right
        newquad.front_top_left[0] = -1*newquad.width/2; 
        newquad.front_top_left[1] =  newquad.height/2; 
        newquad.front_top_left[2] =  newquad.depth/2; 
        //front side: top left


        heat_sink_quads_.push_back(newquad);
    }

    for (int i=0;i<9;i++) {
        for (int j=0;j<heat_spreader_fins;j++) {

            Quad newquad;
            newquad.width=1;
            newquad.height=5;
            newquad.depth=1;
            newquad.color0[3]=0.3;
            newquad.color0[0]=0.2;
            newquad.color0[2]=0.55;
            newquad.color0[1]=0.80;
            if (j%2!=0)
                newquad.quad_defined=false;
            //back side: bottomleft
            newquad.back_bottom_left[0] = -1*newquad.width/2; 
            newquad.back_bottom_left[1] = -1*newquad.height/2; 
            newquad.back_bottom_left[2] = -1*newquad.depth/2; 
            newquad.back_bottom_right[0] =  newquad.width/2; 
            newquad.back_bottom_right[1] = -1*newquad.height/2; 
            newquad.back_bottom_right[2] = -1*newquad.depth/2; 
            //back side: bottomright
            newquad.back_top_right[0] =  newquad.width/2; 
            newquad.back_top_right[1] =  newquad.height/2; 
            newquad.back_top_right[2] = -1*newquad.depth/2; 
            //back side: top right
            newquad.back_top_left[0] = -1*newquad.width/2; 
            newquad.back_top_left[1] =  newquad.height/2; 
            newquad.back_top_left[2] = -1*newquad.depth/2; 
            //back side: top left
            newquad.front_bottom_left[0] = -1*newquad.width/2; 
            newquad.front_bottom_left[1] = -1*newquad.height/2; 
            newquad.front_bottom_left[2] =  newquad.depth/2; 
            //front side: bottomleft
            newquad.front_bottom_right[0] = newquad.width/2; 
            newquad.front_bottom_right[1] = -1*newquad.height/2; 
            newquad.front_bottom_right[2] =  newquad.depth/2; 
            //front side: bottomright
            newquad.front_top_right[0] =  newquad.width/2; 
            newquad.front_top_right[1] =  newquad.height/2; 
            newquad.front_top_right[2] =  newquad.depth/2; 
            //front side: top right
            newquad.front_top_left[0] = -1*newquad.width/2; 
            newquad.front_top_left[1] =  newquad.height/2; 
            newquad.front_top_left[2] =  newquad.depth/2; 
            //front side: top left

            heat_sink_fins_quads_.push_back(newquad);
        }
    }

    //set the rotation matrix to it's initial identity matrix
    memset(the_rotMatrix, 0, sizeof(the_rotMatrix));
    the_rotMatrix[0]=1;      
    the_rotMatrix[5]=1;      
    the_rotMatrix[10]=1;         
    the_rotMatrix[15]=1;         
}
void SESCthermView::drawQuads(){
    glPushMatrix();
    glTranslatef(xshift, yshift, 0);
    glMultMatrixf(the_rotMatrix);
    glScalef(float(size),float(size),float(size));


    glTranslatef(-1*chip_width/2, 0, 1*chip_height/2); //go to the bottom left corner of the chip (looking down)
    //draw the chip layer
    int itor=0;
    glCallList(chip_dl_);
    /*
    for (double y=0;y<chip_height;y+=chip_quads_[itor-1].height) {
        for (double x=0;x<chip_width;x+=chip_quads_[itor].width) {
            drawQuad(chip_quads_[itor]);
            //glCallList(chip_quads_[itor].display_list);
            glTranslatef(chip_quads_[itor].width, 0, 0);
            itor++;
        }

        glTranslatef(-1*chip_width, 0, -1*chip_quads_[itor-1].height);
    }
    */
    glTranslatef(chip_width/2,0,chip_height/2);
    glTranslatef(-1*heat_spreader_width/2,-1*chip_thickness/2-heat_spreader_thickness/2,heat_spreader_height/2);


    glCallList(heat_spreader_dl_);
    /*
    itor=0;
    //draw the heat spreader layer
    for (double y=0;y<heat_spreader_height;y+=heat_spreader_quads_[itor-1].height) {
        for (double x=0;x<heat_spreader_width;x+=heat_spreader_quads_[itor].width) {
            drawQuad(heat_spreader_quads_[itor]);

            glTranslatef(heat_spreader_quads_[itor].width, 0, 0);
            itor++;
        }


        glTranslatef(-1*heat_spreader_width, 0, -1*heat_spreader_quads_[itor-1].height);

    }
    */
    glTranslatef(heat_spreader_width/2,0,heat_spreader_height/2);

    glTranslatef(-1*heat_sink_width/2,-1*heat_spreader_thickness/2-heat_sink_thickness/2,heat_sink_height/2);

    glCallList(  heat_sink_dl_);
    /*
    itor=0;

    //draw the heat sink layer
    for (double y=0;y<heat_sink_height;y+=heat_sink_quads_[itor-1].height) {
        for (double x=0;x<heat_sink_width;x+=heat_sink_quads_[itor].width) {
            drawQuad(heat_sink_quads_[itor]);
            glTranslatef(heat_sink_quads_[itor].width, 0, 0);
            itor++;
        }
        glTranslatef(-1*heat_sink_width, 0, -1*heat_sink_quads_[itor-1].height);
    }
    */
    glTranslatef(heat_sink_width/2,0,heat_sink_height/2);
    glTranslatef(-1*heat_sink_width/2,-1*heat_sink_thickness/2-heat_sink_fins_thickness/2,heat_sink_height/2);

    glCallList(  heat_sink_fins_dl_);
    /*
    itor=0;
    //draw the heat sink fins layer
    for (double y=0;y<heat_sink_height;y+=heat_sink_quads_[itor-1].height) {
        for (double x=0;x<heat_sink_width;x+=heat_sink_quads_[itor].width) {
            if (heat_sink_fins_quads_[itor].quad_defined)
                drawQuad(heat_sink_fins_quads_[itor]);
            glTranslatef(heat_sink_fins_quads_[itor].width, 0, 0);
            itor++;
        }
        glTranslatef(-1*heat_sink_width, 0, -1*heat_sink_quads_[itor-1].height);
    }
    */
    //glTranslatef(heat_sink_width/2,0,heat_sink_height/2);


    glPopMatrix();






}
void SESCthermView::drawQuad(Quad& quad){
/* Draw a colored cube */
#define ALPHA 0.5
    glShadeModel(GL_FLAT);

    //back side
    glBegin(GL_QUADS);
    // glColor4f(quad.color0[0],quad.color0[1],quad.color0[2],quad.color0[3]);
    glBindTexture(GL_TEXTURE_2D,quad.texture[0]);
    glTexCoord2d(0.0f,0.0f);glVertex3fv(quad.back_bottom_left);
    glTexCoord2d(1.0f,0.0f);glVertex3fv(quad.back_bottom_right);
    glTexCoord2d(1.0f,1.0f);glVertex3fv(quad.back_top_right);
    glTexCoord2d(0.0f,1.0f);glVertex3fv(quad.back_top_left);

    //bottom side
    //     glColor4f(quad.color0[0],quad.color0[1],quad.color0[2],quad.color0[3]);
    glBindTexture(GL_TEXTURE_2D,quad.texture[1]);
    glTexCoord2d(0.0f,0.0f);;glVertex3fv(quad.front_bottom_left);
    glTexCoord2d(1.0f,0.0f);glVertex3fv(quad.front_bottom_right);
    glTexCoord2d(1.0f,1.0f);glVertex3fv(quad.back_bottom_right);
    glTexCoord2d(0.0f,1.0f);glVertex3fv(quad.back_bottom_left);

    //top side
    //   glColor4f(quad.color0[0],quad.color0[1],quad.color0[2],quad.color0[3]);
    glBindTexture(GL_TEXTURE_2D,quad.texture[2]);
    glTexCoord2d(0.0f,0.0f);glVertex3fv(quad.front_top_left);
    glTexCoord2d(1.0f,0.0f);glVertex3fv(quad.front_top_right);
    glTexCoord2d(1.0f,1.0f);glVertex3fv(quad.back_top_right);
    glTexCoord2d(0.0f,1.0f);glVertex3fv(quad.back_top_left);

    //front side
    //     glColor4f(quad.color0[0],quad.color0[1],quad.color0[2],quad.color0[3]);
    glBindTexture(GL_TEXTURE_2D,quad.texture[3]);
    glTexCoord2d(0.0f,0.0f);glVertex3fv(quad.front_bottom_left);
    glTexCoord2d(1.0f,0.0f);glVertex3fv(quad.front_bottom_right);
    glTexCoord2d(1.0f,1.0f);;glVertex3fv(quad.front_top_right);
    glTexCoord2d(0.0f,1.0f);glVertex3fv(quad.front_top_left);

    //left side
    //     glColor4f(quad.color0[0],quad.color0[1],quad.color0[2],quad.color0[3]);
    glBindTexture(GL_TEXTURE_2D,quad.texture[4]);
    glTexCoord2d(0.0f,0.0f);glVertex3fv(quad.back_bottom_left);
    glTexCoord2d(1.0f,0.0f);glVertex3fv(quad.front_bottom_left);
    glTexCoord2d(1.0f,1.0f);glVertex3fv(quad.front_top_left);
    glTexCoord2d(0.0f,1.0f);glVertex3fv(quad.back_top_left);

    //right side
    //      glColor4f(quad.color0[0],quad.color0[1],quad.color0[2],quad.color0[3]);
    glBindTexture(GL_TEXTURE_2D,quad.texture[5]);
    glTexCoord2d(0.0f,0.0f);glVertex3fv(quad.front_bottom_right);
    glTexCoord2d(1.0f,0.0f);glVertex3fv(quad.back_bottom_right);
    glTexCoord2d(1.0f,1.0f);glVertex3fv(quad.back_top_right);
    glTexCoord2d(0.0f,1.0f);glVertex3fv(quad.front_top_right);
    glEnd();

/*
    glBegin(GL_LINES);
    glColor3f(1.0, 1.0, 1.0);
    glVertex3fv(quad.back_bottom_left);
    glVertex3fv(quad.back_bottom_right);

    glVertex3fv(quad.back_bottom_right);
    glVertex3fv(quad.back_top_right);

    glVertex3fv(quad.back_top_right);
    glVertex3fv(quad.back_top_left);

    glVertex3fv(quad.back_top_left);
    glVertex3fv(quad.back_bottom_left);

    glVertex3fv(quad.front_bottom_left);
    glVertex3fv(quad.front_bottom_right);

    glVertex3fv(quad.front_bottom_right);
    glVertex3fv(quad.front_top_right);

    glVertex3fv(quad.front_top_right);
    glVertex3fv(quad.front_top_left);

    glVertex3fv(quad.front_top_left);
    glVertex3fv(quad.front_bottom_left);

    glVertex3fv(quad.back_bottom_left);
    glVertex3fv(quad.front_bottom_left);

    glVertex3fv(quad.back_bottom_right);
    glVertex3fv(quad.front_bottom_right);

    glVertex3fv(quad.back_top_right);
    glVertex3fv(quad.front_top_right);

    glVertex3fv(quad.back_top_left);
    glVertex3fv(quad.front_top_left);
    glEnd();
*/
}//drawCube

void SESCthermView::draw(){
    if (!valid()) {
        glLoadIdentity();
        glViewport(0,0,w(),h());
        glGetIntegerv(GL_VIEWPORT, vp);
        glOrtho(-10,10,-10,10,-20030,10000);
        //    glEnable(GL_BLEND);
        //  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        //  glEnable(GL_LINE_SMOOTH);
        glEnable(GL_TEXTURE_2D);
        //  glHint(GL_LINE_SMOOTH_HINT, GL_DONT_CARE);
        glClearDepth(1.0);               // Enables Clearing Of The Depth Buffer

        //    glDepthFunc(GL_LEQUAL);          // The Type Of Depth Test To Do
        glEnable(GL_DEPTH_TEST);            // Enables Depth Testing
        glDepthFunc(GL_LESS); 
        //    glShadeModel(GL_SMOOTH);            // Enables Smooth Color Shading
        //   glShadeModel(GL_FLAT);
        glMatrixMode(GL_MODELVIEW);
        //glEnable(GL_COLOR_MATERIAL);
        if (glXGetCurrentContext()==NULL) {
            cerr << "FATAL: Could not obtain rendering context. Exiting." << endl;
            exit(1);
        }
//create textures
        for (int i=0;i<chip_quads_.size();i++) {

            glGenTextures(1, chip_quads_[i].texture);

            glBindTexture(GL_TEXTURE_2D, chip_quads_[i].texture[0]);
            // glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_NEAREST);
            // glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
            // glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);
            // glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP);


/*
            gluBuild2DMipmaps(GL_TEXTURE_2D,     // texture to specify
                              3,           // internal texture storage format
                              64,             // texture width
                              64,            // texture height
                              GL_RGB,           // pixel format
                              GL_UNSIGNED_BYTE,  // color component format
                              chip_quads_[i].down_data);    // pointer to texture image
*/
            glTexImage2D(GL_TEXTURE_2D,0,3,64,64,0,GL_RGB,GL_UNSIGNED_BYTE, chip_quads_[i].down_data);
            // delete chip_quads[0].data;
        }


        for (int i=0;i<heat_spreader_quads_.size();i++) {

            glGenTextures(1, heat_spreader_quads_[i].texture);

            glBindTexture(GL_TEXTURE_2D, heat_spreader_quads_[i].texture[0]);
            // glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_NEAREST);
            // glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
            // glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);
            // glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP);



            gluBuild2DMipmaps(GL_TEXTURE_2D,     // texture to specify
                              3,           // internal texture storage format
                              64,             // texture width
                              64,            // texture height
                              GL_RGB,           // pixel format
                              GL_UNSIGNED_BYTE,  // color component format
                              heat_spreader_quads_[i].down_data);    // pointer to texture image

            // delete chip_quads[0].data;
        }

        for (int i=0;i<heat_sink_quads_.size();i++) {

            glGenTextures(1, heat_sink_quads_[i].texture);

            glBindTexture(GL_TEXTURE_2D, heat_sink_quads_[i].texture[0]);
            // glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_NEAREST);
            // glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
            // glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);
            // glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP);



            gluBuild2DMipmaps(GL_TEXTURE_2D,     // texture to specify
                              3,           // internal texture storage format
                              64,             // texture width
                              64,            // texture height
                              GL_RGB,           // pixel format
                              GL_UNSIGNED_BYTE,  // color component format
                              heat_sink_quads_[i].down_data);    // pointer to texture image

            // delete chip_quads[0].data;
        }


        for (int i=0;i<heat_sink_fins_quads_.size();i++) {

            glGenTextures(1, heat_sink_fins_quads_[i].texture);

            glBindTexture(GL_TEXTURE_2D, heat_sink_fins_quads_[i].texture[0]);
            // glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_NEAREST);
            // glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
            // glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP);
            // glTexParameterf(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP);



            gluBuild2DMipmaps(GL_TEXTURE_2D,     // texture to specify
                              3,           // internal texture storage format
                              64,             // texture width
                              64,            // texture height
                              GL_RGB,           // pixel format
                              GL_UNSIGNED_BYTE,  // color component format
                              heat_sink_fins_quads_[i].down_data);    // pointer to texture image

            // delete chip_quads[0].data;
        }

        for (int i=0;i<chip_quads_.size();i++) {
            chip_quads_[i].display_list = glGenLists(1);
            glNewList(chip_quads_[i].display_list, GL_COMPILE);
            drawQuad(chip_quads_[i]);
            glEndList();
        }

        for (int i=0;i<heat_spreader_quads_.size();i++) {
            heat_spreader_quads_[i].display_list = glGenLists(1);
            glNewList(heat_spreader_quads_[i].display_list, GL_COMPILE);
            drawQuad(heat_spreader_quads_[i]);
            glEndList();
        }

        for (int i=0;i<heat_sink_quads_.size();i++) {
            heat_sink_quads_[i].display_list = glGenLists(1);
            glNewList(heat_sink_quads_[i].display_list, GL_COMPILE);
            drawQuad(heat_sink_quads_[i]);
            glEndList();
        }

        for (int i=0;i<heat_sink_fins_quads_.size();i++) {
            heat_sink_fins_quads_[i].display_list = glGenLists(1);
            glNewList(heat_sink_fins_quads_[i].display_list, GL_COMPILE);
            drawQuad(heat_sink_fins_quads_[i]);
            glEndList();
        }

        chip_dl_=glGenLists(1);
        glNewList(chip_dl_, GL_COMPILE);
        int itor=0;
        for (double y=0;y<chip_height;y+=chip_quads_[itor-1].height) {
            for (double x=0;x<chip_width;x+=chip_quads_[itor].width) {
                glCallList(chip_quads_[itor].display_list);
                glTranslatef(chip_quads_[itor].width, 0, 0);
                itor++;
            }

            glTranslatef(-1*chip_width, 0, -1*chip_quads_[itor-1].height);
        }
        glEndList();


        heat_spreader_dl_=glGenLists(1);
        glNewList(heat_spreader_dl_, GL_COMPILE);
        itor=0;
        //draw the heat spreader layer
        for (double y=0;y<heat_spreader_height;y+=heat_spreader_quads_[itor-1].height) {
            for (double x=0;x<heat_spreader_width;x+=heat_spreader_quads_[itor].width) {
                drawQuad(heat_spreader_quads_[itor]);

                glTranslatef(heat_spreader_quads_[itor].width, 0, 0);
                itor++;
            }


            glTranslatef(-1*heat_spreader_width, 0, -1*heat_spreader_quads_[itor-1].height);





        }
        glEndList();

        heat_sink_dl_=glGenLists(1);
        glNewList(heat_sink_dl_, GL_COMPILE);
        itor=0;

        //draw the heat sink layer
        for (double y=0;y<heat_sink_height;y+=heat_sink_quads_[itor-1].height) {
            for (double x=0;x<heat_sink_width;x+=heat_sink_quads_[itor].width) {
                drawQuad(heat_sink_quads_[itor]);
                glTranslatef(heat_sink_quads_[itor].width, 0, 0);
                itor++;
            }
            glTranslatef(-1*heat_sink_width, 0, -1*heat_sink_quads_[itor-1].height);
        }
        glEndList();


        heat_sink_fins_dl_=glGenLists(1);
        glNewList(heat_sink_fins_dl_, GL_COMPILE);
        itor=0;
        //draw the heat sink fins layer
        for (double y=0;y<heat_sink_height;y+=heat_sink_quads_[itor-1].height) {
            for (double x=0;x<heat_sink_width;x+=heat_sink_quads_[itor].width) {
                if (heat_sink_fins_quads_[itor].quad_defined)
                    drawQuad(heat_sink_fins_quads_[itor]);
                glTranslatef(heat_sink_fins_quads_[itor].width, 0, 0);
                itor++;
            }
            glTranslatef(-1*heat_sink_width, 0, -1*heat_sink_quads_[itor-1].height);
        }
        glEndList();
    }
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


    drawQuads();
}


/* Project the given window point onto the rotation sphere */
void SESCthermView::projectToSphere(int x, int y, double * result){
    /* Calculate the x and y components */
    int width  = vp[2]-vp[0];
    int height = vp[3]-vp[1];
    double vx = ((2.0 * x) - width) / width;
    double vy = ((2.0 * y) - height) / height;

    /* And their length */
    double dxy = sqrt(vx*vx + vy*vy);

    /* Now calc the z component */
    double vz = 0;
    if (dxy < 1)
        vz = sqrt(1.0 - dxy);

    /* Calculate the normalizing factor */
    double norm = 1.0 / sqrt(vx*vx + vy*vy + vz*vz);

    /* Store the normalized result */
    result[0] = vx * norm;
    result[1] = vy * norm;
    result[2] = vz * norm;
}

/* Rotate to the given window point */
void SESCthermView::trackballRotate(int x, int y){
    /* Project the current point onto the sphere */
    projectToSphere(x, y, new_vector);

    /* Calculate the distance it's moved */
    double dx = new_vector[0] - old_vector[0];
    double dy = new_vector[1] - old_vector[1];
    double dz = new_vector[2] - old_vector[2];

    /* Update the number of degrees of rotation */
    double angle = 90 * sqrt(dx*dx + dy*dy + dz*dz);

    /* Setup the axis to rotate around (cross product of the old and
     * new mouse vectors)
     */
    double ax = old_vector[1]*new_vector[2] - old_vector[2]*new_vector[1];
    double ay = old_vector[2]*new_vector[0] - old_vector[0]*new_vector[2];
    double az = old_vector[0]*new_vector[1] - old_vector[1]*new_vector[0];

    /* Now apply the new rotation... */
    glPushMatrix();
    glLoadIdentity();
    glRotatef(angle, ax, ay, az);

    /* Extract the per-axis rotation and upset our GUI to show it */
    GLfloat tmp[16];
    glGetFloatv(GL_MODELVIEW_MATRIX, tmp);
    adjustRotFromMatrix(tmp);

    /* Add in the rest of the accumulated matrix and save the result */
    glMultMatrixf(the_rotMatrix);
    glGetFloatv(GL_MODELVIEW_MATRIX, the_rotMatrix);
    glPopMatrix();

    /* Move the new position to the old... */
    old_vector[0] = new_vector[0];
    old_vector[1] = new_vector[1];
    old_vector[2] = new_vector[2];
}
//THIS MAY ONLY UPDATE SLIDERS?!?
/* Adjust our rotation amounts to take into account the new rotation in
 * the given matrix
 */
enum {
    X_AXIS = 0,
    Y_AXIS = 1,
    Z_AXIS = 2
};
void SESCthermView::adjustRotFromMatrix(GLfloat *rotMatrix){
    /* Extract the x rotation and save it */
    double xRot = asin(rotMatrix[6]) * (180 / M_PI);
    adjustRotState(X_AXIS, xRot);

    /* Same for Y */
    double yRot = atan2(-rotMatrix[2], rotMatrix[10]) * (180 / M_PI);
    adjustRotState(Y_AXIS, yRot);

    /* And Z */
    double zRot = atan2(-rotMatrix[4], rotMatrix[5]) * (180 / M_PI);
    adjustRotState(Z_AXIS, zRot);

    /* Update the UI */
    //refreshOwner();
}
/* Keep track of how much rotation we've done */
void SESCthermView::adjustRotState(int axis, double degrees){
    /* Update the current position */
    rot_values[axis] += degrees;
    if (rot_values[axis] > 360)
        rot_values[axis] -= 360;
    else if (rot_values[axis] < -360)
        rot_values[axis] += 360;
}

int SESCthermView::handle(int event){
    bool handled = false;
    int x = Fl::event_x();
    int y = vp[3] - Fl::event_y();

    /* Handle each button */
    switch (Fl::event_button()) {
    case FL_LEFT_MOUSE:{
            switch (event) {
            case FL_PUSH:
                projectToSphere(x, y, old_vector);
                return(true);
                break;

            case FL_RELEASE:
                return(true);
                break;

            case FL_DRAG:
                trackballRotate(x, y);
                redraw();   
                return(true);
                break;
            }
        }
    case FL_MIDDLE_MOUSE:
        handled = false;
        break;

    case FL_RIGHT_MOUSE:
        handled = false;
        break;
    }

    /* If we handled it, return 1 */
    if (handled) {
        redraw();
        return(1);
    }
    /* If we didn't handle it, pass it on */
    return(Fl_Gl_Window::handle(event));
}







