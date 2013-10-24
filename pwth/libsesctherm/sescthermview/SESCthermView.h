

#ifndef ESESCTHERMVIEW_H
    #define ESESCTHERMVIEW_H 1
#endif

#include <FL/Fl.H>
#include <FL/Fl_Gl_Window.H>
#include <GL/glx.h>
#include <GL/gl.h>
#include <GL/glu.h>

#include <stdlib.h>
#include <vector>
#include <complex>
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <math.h>
#include <sstream>
#include <string>
#include <stdexcept>
#include <fstream>
#include <sstream>
using namespace std;



//this defines the coordinates of a quad
class Quad {
public:
    float back_bottom_left[3];
    float back_bottom_right[3];
    float back_top_right[3];
    float back_top_left[3];
    float front_bottom_left[3];
    float front_bottom_right[3];
    float front_top_right[3];
    float front_top_left[3];
    float color0[4];
    float width;
    float depth;
    float height;
    bool quad_defined;
    GLuint texture[6]; //allocate indices to 6 textures
    GLuint display_list; //display list index
    unsigned char* down_data; //allocate space for the texture data
    unsigned char* up_data; //allocate space for the texture data
    unsigned char* left_data; //allocate space for the texture data
    unsigned char* right_data; //allocate space for the texture data
    unsigned char* top_data; //allocate space for the texture data
    unsigned char* bottom_data; //allocate space for the texture data
    Quad(){
        /*
        boxv0[2]=0;
        boxv1[2]=0;
        boxv2[2]=0;
        boxv3[2]=0;
        boxv4[2]=0;
        boxv5[2]=0;
        boxv6[2]=0;
        boxv7[2]=0;*/
        width=0;
        depth=0;
        height=0;
        quad_defined=true;
    }

};
class ESESCthermView : public Fl_Gl_Window {

public:
    // this value determines the scaling factor used to draw the cube.
    double size;

    /**************************************************************
    *  Constructor
    **************************************************************/
    ESESCthermView(int x,int y,int w,int h,const char *l=0);

    /************************************************************
     * Sets the x shift of the cube view camera.
     *
     * 	This function is called by the slider in ESESCthermViewUI and the
     *  initialize button in ESESCthermViewUI.
     ************************************************************/
    void panx(float x){xshift=x;};

    /************************************************************ 
     *  Sets the y shift of the cube view camera.
     *
     * This function is called by the slider in ESESCthermViewUI and the
     * initialize button in ESESCthermViewUI.
     ************************************************************/
    void pany(float y){yshift=y;};

    /********************************************************
     * The widget class draw() override.
     *
     *The draw() function initialize Gl for another round o f drawing
     * then calls specialized functions for drawing each of the
     * entities displayed in the cube view.
     *
     ******************************************************************/
    void draw();   


    /*****************************************************************
    *  Trackball routines written by Daniel Keller
    *******************************************************************/
    void     projectToSphere(int x, int y, double *result);
    void     trackballRotate(int x, int y);
    void     adjustRotFromMatrix(GLfloat *rotMatrix); 
    void     adjustRotState(int axis, double degrees); 
    /**************************************************************
    *  Handles FLTK events
    ***************************************************************/
    int      handle(int event);

private:

    /**********************************************************  
     *  Draw the cube boundaries
     *
     *  Draw the faces of the cube using the boxv[] vertices, using
     * GL_LINE_LOOP for the faces. The color is \#defined by CUBECOLOR.
     **********************************************************/
    void drawQuad(Quad&);
    void genTextureQuad(Quad& quad, 
                         unsigned char* centertemp, 
                         unsigned char* lefttemp, 
                         unsigned char* righttemp, 
                         unsigned char* toptemp, 
                         unsigned char* bottomtemp, 
                         unsigned char* downtemp, 
                         unsigned char* uptemp);
    void drawQuads();
    float xshift,yshift;

    enum {
        X_AXIS = 0,
        Y_AXIS = 1,
        Z_AXIS = 2
    };
    /******************************************************
    * These values are used to set up the rotational matrix 
    * for the trackball
    *******************************************************/
    GLfloat rot_values[3];
    GLint vp[4];    //holds viewport
    double old_vector[3];
    double new_vector[3];
    GLfloat the_rotMatrix[16];
    vector<Quad> chip_quads_;
    vector<Quad> heat_spreader_quads_;
    vector<Quad> heat_sink_quads_;
    vector<Quad> heat_sink_fins_quads_;
    GLuint chip_dl_;
    GLuint heat_spreader_dl_;
    GLuint heat_sink_dl_;
    GLuint heat_sink_fins_dl_;
};


