/*
 *  align_image_model.h
 *  FaceTracker
 *
 *  Created by Roy Shilkrot on 10/25/11.
 *  Copyright 2011 MIT. All rights reserved.
 *
 */
#pragma once

#include "opencv2/opencv.hpp"

using namespace cv;

#if defined(__APPLE__)
#  ifdef USE_GLEW
#    include <GL/glew.h>        // OpenGL Extension Wrangler library
#  else
#    include <OpenGL/gl.h>      // OpenGL library
#  endif
//#  include <GLUT/glut.h>        // OpenGL Utility Toolkit
#	 include <GL/freeglut.h>
#elif defined(__linux__)
#  ifdef USE_GLEW
#	 include <GL/glew.h>        // OpenGL Extension Wrangler library
#  else
#    define GL_GLEXT_PROTOTYPES
#	 include <GL/gl.h>          // OpenGL library
#    include <GL/glext.h>       // OpenGL extension prototypes
#  endif
//#  include <GL/glut.h>          // OpenGL Utility Toolkit
#	 include <GL/freeglut.h>
#else
#  ifdef USE_GLEW
#    include <glew.h>           // OpenGL Extension Wrangler library
#  else
#    include <gl.h>             // OpenGL library
#  endif
//#  include <glut.h>             // OpenGL Utility Toolkit
#  include <freeglut.h>
#endif

#include "glm.h"

#include <fstream>

#include "glow.h"
GLOW_NAMESPACE_USING

using namespace std;

#include "spherical_harmonics_analysis.h"

#define RAD_TO_DEG 57.2957795
#define DEG_TO_RAD 0.0174532925
#define PI 3.14159265

#include "../VirtualSurgeon/VirtualSurgeon_Utils/VirtualSurgeon_Utils.h"

#include "OGL_OCV_common.h"

typedef struct my_texture {
	GLuint tex_id;
	double twr,thr,aspect_w2h;
	my_texture():tex_id(-1),twr(1.0),thr(1.0) {}
} MyTexture;

//#define IMAGE_FILENAME1 "/Users/royshilkrot/Downloads/Head Replacement/VirtualSurgeon/images/2579685145_4abb9337e5_d.jpg"
//#define IMAGE_FILENAME2 "/Users/royshilkrot/Downloads/Head Replacement/VirtualSurgeon/images/1936149265_a1fdc856b8_b_d.jpg"

#define STATE_IDLE 0
#define STATE_MOVING 1
#define STATE_SCALING 2
#define STATE_ROTATING_XY 3
#define STATE_ROTATING_Z 4

#define SIGN(x) ((x)>0?1:-1)

typedef struct face_orientation {
	double face_scale;
	double face_x_rot;
	double face_y_rot;
	double face_z_rot;
	Point2d face_pos;
	
	void saveToFile(const string& filename) {
		FileStorage fs(filename, FileStorage::WRITE);
		fs << "face_scale" << face_scale;
		fs << "face_x_rot" << face_x_rot;
		fs << "face_y_rot" << face_y_rot;
		fs << "face_z_rot" << face_z_rot;
		fs << "face_pos_X" << face_pos.x;
		fs << "face_pos_Y" << face_pos.y;
		fs.release();
	};
	
	void readFromFile(const string& filename) {
		FileStorage fs(filename, FileStorage::READ);
		fs["face_scale"] >> face_scale;
		fs["face_x_rot"] >> face_x_rot;
		fs["face_y_rot"] >> face_y_rot;
		fs["face_z_rot"] >> face_z_rot;
		fs["face_pos_X"] >> face_pos.x;
		fs["face_pos_Y"] >> face_pos.y;
		fs.release();
	};
	
	friend std::ostream& operator<< (std::ostream& o, struct face_orientation const& fo) {
		return  o << "FaceOrientation: ["<<fo.face_pos<<",scale="<<fo.face_scale<<",x_rot="<<fo.face_x_rot<<",y_rot="<<fo.face_y_rot<<",z_rot="<<fo.face_z_rot<<"]\n";
	}
} FaceOrientation;

class AFTMWindowHandler {
public:
	virtual void OnEndPaint()=0;
	virtual void OnReshape(int w,int h)=0;
	virtual void OnMouseDown(Glow::MouseButton b, int x, int y, Glow::Modifiers m)=0;
	virtual void OnMouseUp(Glow::MouseButton b, int x, int y, Glow::Modifiers m)=0;
	virtual void OnMouseDrag(int x, int y)=0;
	virtual void OnKeyboard(Glow::KeyCode k, int x, int y, Glow::Modifiers m)=0;
};

class AFTMGlowWindow : public GlowWindow {
	AFTMWindowHandler* model;
public:	
	AFTMGlowWindow(const string& winname, int i1, int i2, int i3, int i4, Glow::BufferType i5, Glow::EventMask i6):
	GlowWindow(winname.c_str(), i1, i2, i3, i4, i5, i6) {};
	
	void setModel(AFTMWindowHandler* _model) { model = _model;}
	
	void OnEndPaint() {
		model->OnEndPaint();
	}
	virtual void OnReshape(int w,int h) {
		model->OnReshape(w,h);
	}
	virtual void OnMouseDown(Glow::MouseButton b, int x, int y, Glow::Modifiers m) {
		model->OnMouseDown(b,x,y,m);
	}
	virtual void OnMouseUp(Glow::MouseButton b, int x, int y, Glow::Modifiers m) {
		model->OnMouseUp(b,x,y,m);
	}
	virtual void OnMouseDrag(int x, int y) {
		model->OnMouseDrag(x,y);
	}
	void OnKeyboard(Glow::KeyCode k, int x, int y, Glow::Modifiers m) {
		model->OnKeyboard(k,x,y,m);
	}
};

class AlignFaceToModel : public AFTMWindowHandler {
private:
	AFTMGlowWindow* glowWindow;
	
	GLMmodel* head_obj;
		
	Mat tex_img[2];
	MyTexture image_tex[2];
	int currentFace;
	
	int state;
	
	GLvoid* my_program; //TODO: release it when done
	bool useShaders;
	
	bool doRelighting;
	
	Ptr<SphericalHarmonicsAnalyzer> align_sha[2];
	
	FaceOrientation faces[2];
	
	bool face_opaque;
	bool showFace;
	Point start,last;
	bool mouse_down;
	Vec3d d0v;
	
	int glutwin;
	
	string IMAGE_FILENAME1,IMAGE_FILENAME2;
	
	Mat_<Vec3f> relitFace;
	Mat_<uchar> relitMask;
	Rect bound;
public:
	AlignFaceToModel(const VirtualSurgeon::VirtualSurgeonFaceData& face_data, Mat& face_image):
		IMAGE_FILENAME1(face_data.filename)
	{
		glowWindow = new AFTMGlowWindow("AlignFaceToModel", 
									GlowWindow::autoPosition, GlowWindow::autoPosition, //position
									GlowSubwindow::parentWindowSize,GlowSubwindow::parentWindowSize, //size 
									Glow::rgbBuffer | Glow::doubleBuffer | Glow::depthBuffer, //flags
									Glow::allEvents );
		glowWindow->setModel(this);
		
		for (int i=0; i<2; i++) {
			align_sha[i] = Ptr<SphericalHarmonicsAnalyzer>(NULL);
			faces[i] = (FaceOrientation){100.0,0.0,0.0,0.0,(face_data.li+face_data.ri) * 0.5};
		}
		useShaders = false;
		doRelighting = false;
		currentFace = 0;
		state = STATE_IDLE;
		face_opaque = false;
		showFace = true;
		mouse_down = false;
		
		face_image.copyTo(tex_img[0]);
		align_init();
		
		glowWindow->Reshape(tex_img[0].cols, tex_img[0].rows);
	}
	
	const Mat_<Vec3f>& getReLitFace() { return relitFace; }
	const Mat_<uchar>& getReLitMask() { return relitMask; }
	const Rect& getReLitRect() { return bound; }
	
	void checkARBError(GLvoid* obj) {
		char infolog[1024] = {0}; int _written = 0;
		glGetInfoLogARB(obj, 1024, &_written, infolog);
		if(_written>0) {
			cerr << infolog << endl;
		}
	}	
	
	static bool notIsAscii(int i) { return !isascii(i); }
	
	void align_init_shaders() {
		const GLubyte* lang_ver = glGetString(GL_SHADING_LANGUAGE_VERSION);
		cout <<"shading language version: "<<(uchar*)lang_ver<<endl;
		
		const char * my_fragment_shader_source;
		const char * my_vertex_shader_source;
		
		string _file = __FILE__;
		string _dir = _file.substr(0,_file.rfind("/")) + "/";
		ifstream ifs(string(_dir + "vshader.txt").c_str());
		ostringstream ss; ss << ifs.rdbuf();
		ifstream ifs1(string(_dir + "fshader.txt").c_str());
		ostringstream ss1; ss1 << ifs1.rdbuf();
		
		ifs.close(); ifs1.close();
		
		string _vertex = ss.str(); _vertex.erase(remove_if(_vertex.begin(), _vertex.end(), notIsAscii), _vertex.end());
		string _frag = ss1.str(); _frag.erase(remove_if(_frag.begin(), _frag.end(), notIsAscii), _frag.end());
		
		// Get Vertex And Fragment Shader Sources
		my_fragment_shader_source = _frag.c_str();
		my_vertex_shader_source = _vertex.c_str();
		
//		cout << "vertex shader:"<<endl<<my_vertex_shader_source<<endl;
//		cout << "fragment shader:"<<endl<<my_fragment_shader_source<<endl;
		
		GLvoid* my_vertex_shader;
		GLvoid* my_fragment_shader;
		
		// Create Shader And Program Objects
		my_program = glCreateProgramObjectARB();
		my_vertex_shader = glCreateShaderObjectARB(GL_VERTEX_SHADER_ARB);
		my_fragment_shader = glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);
		
		// Load Shader Sources
		glShaderSourceARB(my_vertex_shader, 1, &my_vertex_shader_source, NULL);
		checkARBError(my_vertex_shader);
		glShaderSourceARB(my_fragment_shader, 1, &my_fragment_shader_source, NULL);
		checkARBError(my_fragment_shader);
		
		
		// Compile The Shaders
		glCompileShaderARB(my_vertex_shader);
		checkARBError(my_vertex_shader);
		glCompileShaderARB(my_fragment_shader);
		checkARBError(my_fragment_shader);
		
		// Attach The Shader Objects To The Program Object
		glAttachObjectARB(my_program, my_vertex_shader);
		glAttachObjectARB(my_program, my_fragment_shader);
		checkARBError(my_program);
		
		// Link The Program Object
		glLinkProgramARB(my_program);
		checkARBError(my_program);
		
		// Use The Program Object Instead Of Fixed Function OpenGL
		//	glUseProgramObjectARB(my_program);
	}
	
	void align_init() {
		{
//			tex_img[0] = imread(IMAGE_FILENAME1);
			makePow2Texture(tex_img[0], &(image_tex[0].tex_id), &(image_tex[0].twr), &(image_tex[0].thr));
			image_tex[0].aspect_w2h = (double)tex_img[0].cols/(double)tex_img[0].rows;
		}
//		{
//			tex_img[1] = imread(IMAGE_FILENAME2);
//			makePow2Texture(tex_img[1], &(image_tex[1].tex_id), &(image_tex[1].twr), &(image_tex[1].thr));
//			image_tex[1].aspect_w2h = (double)tex_img[1].cols/(double)tex_img[1].rows;
//		}
		//head_obj = glmReadOBJ("/Users/royshilkrot/Downloads/torso_uniform.off/face.obj");
		//	head_obj = glmReadOBJ("/Users/royshilkrot/Dropbox/front_cylinder.obj");
		head_obj = glmReadOBJ("/Users/royshilkrot/Dropbox/elipsoid_face_nose.obj");
		glmUnitize(head_obj);
		
		align_init_shaders();
		
		glLightfv(GL_LIGHT0, GL_AMBIENT, Vec4f(.2, .2, .2, 1.0).val);
		glLightfv(GL_LIGHT0, GL_DIFFUSE, Vec4f(.8, .8, .8, 1.0).val);
		glEnable(GL_LIGHT0);
		glEnable(GL_LIGHTING);
		
		glEnable(GL_DEPTH_TEST);
		
		glShadeModel(GL_SMOOTH);                        // Enable Smooth Shading
		glClearColor(0.0f, 0.0f, 0.0f, 0.5f);                   // Black Background
		glClearDepth(1.0f);                         // Depth Buffer Setup
		glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);          // Really Nice Perspective 
		
		
		//	glColorMaterial(GL_FRONT_AND_BACK,GL_AMBIENT_AND_DIFFUSE);
		glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, &(Vec4f(0,0,0,1)[0]));
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, &(Vec4f(.8,.8,.8,.6)[0]));
		
		glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		
		glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
		
		/* Setup the view of the cube. */
		glMatrixMode(GL_PROJECTION);
		//	glLoadIdentity();
		//	gluPerspective( /* field of view in degree */ 60.0,
		//				   /* aspect ratio */ 1.0, //4.0/3.0,
		//				   /* Z near */ 1.0, /* Z far */ 1000.0);
		glOrtho(-tex_img[0].cols, 0, 0, tex_img[0].rows, .1, 1000);
		
		glMatrixMode(GL_MODELVIEW);
		//	glLoadIdentity();
		
		gluLookAt(0.0, 0.0, -1.0,	/* eye is at (0,0,-1) */
				  0.0, 0.0, 0.0,      /* center is at (0,0,0) */
				  0.0, 1.0, 0.0);      /* up is in positive Y direction */		
		
		glLightfv(GL_LIGHT0, GL_POSITION, Vec4f(320, 320, -1, 1.0).val);
		
	}
	
	void align_idle() {
		//	aftm->findFace();
		//	
		//	int idx = aftm->tracker->_clm.GetViewIdx();
		//	Draw(tex_img,aftm->tracker->_shape,get_static_con(),get_static_tri(),aftm->tracker->_clm._visi[idx]);
		//	
		//	copyImgToTex(tex_img, &(image_tex.tex_id));
		//
		//	glutSwapBuffers();
	}
	
	const char* getStateString(int _state) {
		switch (_state) {
			case STATE_MOVING:
				return "Move";
				break;
			case STATE_IDLE:
				return "Idle";
				break;
			case STATE_ROTATING_XY:
				return "Rotate XY";
				break;
			case STATE_ROTATING_Z:
				return "Rotate Z";
				break;
			case STATE_SCALING:
				return "Scale";
				break;
			default:
				return "Unknown";
				break;
		}
	}
	
	
	void drawFaceModel(bool draw_axes) {
		glEnable (GL_BLEND); 
		glPushMatrix();
		glTranslated(0, 0, 100);
		glTranslated(faces[currentFace].face_pos.x, faces[currentFace].face_pos.y, 0);
		glRotated(180, 0, 0, 1);
		glRotated(90, 1, 0, 0);
		
		glRotated(faces[currentFace].face_x_rot, 1, 0, 0);
		glRotated(faces[currentFace].face_y_rot, 0, 1, 0);
		glRotated(faces[currentFace].face_z_rot, 0, 0, 1);
		
		if(draw_axes) {
			//Axes
			glBegin(GL_LINES);
			glEnable(GL_COLOR_MATERIAL);
			glColor3ub(255, 0, 0); glVertex3d(0, 0, 0); glVertex3d(44, 0, 0);
			glColor3ub(0, 255, 0); glVertex3d(0, 0, 0); glVertex3d(0, 44, 0);
			glColor3ub(0, 0, 255); glVertex3d(0, 0, 0); glVertex3d(0, 0, -44);
			glEnd();
		}
		
		//Face
		glScaled(faces[currentFace].face_scale, faces[currentFace].face_scale, faces[currentFace].face_scale);
		glEnable(GL_LIGHTING); glDisable(GL_TEXTURE_2D); glDisable(GL_COLOR_MATERIAL); glEnable(GL_NORMALIZE);
		if (useShaders) { //if enabled use normal-map shader
			glUseProgramObjectARB(my_program);
		}
		glmDraw(head_obj, GLM_SMOOTH, GL_RENDER);
		if (useShaders) {
			glUseProgramObjectARB(0); //disable the shaders
		}
		glPopMatrix();
	}	
	
	void drawFaceImage() {
		glEnable(GL_TEXTURE_2D);
		glDisable(GL_LIGHTING);
		glDisable(GL_BLEND);
		glBindTexture(GL_TEXTURE_2D, image_tex[currentFace].tex_id);
		glPushMatrix();
		//	glTranslated(320, 320, 0);
		//	glRotated(180, 0, 0, 1);
		glColor3ub(255, 255, 255);
		
		int vPort[4]; glGetIntegerv(GL_VIEWPORT, vPort);
		glScaled(vPort[3], vPort[3], 1);
		
		double aw2h = image_tex[currentFace].aspect_w2h, ithr = image_tex[currentFace].thr, itwr = image_tex[currentFace].twr;
		glBegin(GL_QUADS);
		
		glTexCoord2d(0, 0);
		glNormal3d(0, 0, -1); 
		glVertex2d(0, 0);
		
		glTexCoord2d(0, ithr);
		glNormal3d(0, 0, -1); 
		glVertex2d(0, 1); 
		
		glTexCoord2d(itwr, ithr);	
		glNormal3d(0, 0, -1); 
		glVertex2d(aw2h, 1);
		
		glTexCoord2d(itwr, 0);				
		glNormal3d(0, 0, -1); 
		glVertex2d(aw2h, 0); 
		
		glEnd();
		glPopMatrix();
	}
	
	
	void drawGUI() {
		glPushMatrix();
		glDisable(GL_LIGHTING);
		glDisable(GL_BLEND);
		glDisable(GL_TEXTURE_2D);
		glRasterPos2i(10, 10);
		glColor4f(0.0f, 0.0f, 1.0f, 1.0f);
		glutBitmapString(GLUT_BITMAP_TIMES_ROMAN_24, (const unsigned char*)getStateString(state));
		glPopMatrix();
		
		if (mouse_down && state == STATE_ROTATING_Z) {
			int vP[4]; glGetIntegerv(GL_VIEWPORT, vP);
			glPushMatrix();
			glTranslated(start.x, vP[3]-start.y, 0);
			glColor3ub(255, 0, 0);
			glBegin(GL_LINES);
			glVertex2i(0,0);
			glVertex2i(-d0v[0], d0v[1]);
			glEnd();
			glutWireCylinder(norm(Point(d0v[0],d0v[1])), 1, 100, 1);
			glPopMatrix();
		}
	}	
	
	void DoRelighting() {
		//Take pixels of normal-map of face
		useShaders = true;
		drawFaceModel(false);
		useShaders = false;
		int vPort[4]; glGetIntegerv(GL_VIEWPORT, vPort);
		Mat_<Vec3f> face_normals(vPort[3],vPort[2]);
		glReadPixels(0, 0, vPort[2], vPort[3], GL_BGR, GL_FLOAT, face_normals.data);
		flip(face_normals,face_normals,0);
//		imshow("normals", face_normals); waitKey(1);
		
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glEnable2D();
		drawFaceImage();
		glDisable2D();
		glGetIntegerv(GL_VIEWPORT, vPort);
		Mat_<Vec3b> face_image(vPort[3],vPort[2]);
		{
			Mat_<Vec4b> face_image_4b(vPort[3],vPort[2]);
			glReadPixels(0, 0, vPort[2], vPort[3], GL_BGRA, GL_UNSIGNED_BYTE, face_image_4b.data);
			flip(face_image_4b,face_image_4b,0);
			mixChannels(&face_image_4b, 1, &face_image, 1, &(Vec6i(0,0,1,1,2,2)[0]), 3);
		}
//		imshow("image",face_image); waitKey(1);
		
		//Get the mask of the face object
		Mat_<uchar> face_mask(face_normals.size(),0);
		{
			vector<vector<Point> > contours;
			Mat_<Vec3b> rgb(face_normals.size()); face_normals.convertTo(rgb, CV_8UC3, 255.0);
			Mat_<uchar> g(face_normals.size()); cvtColor(rgb, g, CV_BGR2GRAY, 1);
			findContours(g, contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);
			bound = boundingRect(contours[0]);
			drawContours(face_mask, contours, 0, Scalar(255), CV_FILLED);
		}
		
//		imshow("mask", face_mask(bound));
//		imshow("image",face_image(bound));
//		imshow("normals", face_normals(bound)); waitKey(1);
		
		//Send to spharmonics 
		SphericalHarmonicsAnalyzer sha(face_image(bound),face_mask(bound),face_normals(bound));
		sha._debug = false;
		sha.approximateInitialLightingCoeffs();
		sha.computeAlbedo();
//		sha.computeLightingCoefficients();
//		sha.computeAlbedo();
		
		sha.getAlbedo().copyTo(relitFace);
		sha.getMask().copyTo(relitMask);
	}
	
	void OnEndPaint() {
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
		if(doRelighting) {
			DoRelighting();
			
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			doRelighting = false;
		}
		
		if(!useShaders) {
			glEnable2D();
			
			drawFaceImage();
			drawGUI();
			
			glDisable2D();
		}
		
		if (showFace) {
			drawFaceModel(true);
		}	
	}
	
	
	virtual void OnReshape(int width,int height) {
		glViewport(0, 0, width, height);
	}
	
	virtual void OnMouseDown(Glow::MouseButton button, int x, int y, Glow::Modifiers modifiers) {
		if (button == Glow::leftButton) {
			start = last = Point(x,y);
			mouse_down = true;
		}
	}
	
	virtual void OnMouseUp(Glow::MouseButton button, int x, int y, Glow::Modifiers modifiers) {
		mouse_down = false;
		glowWindow->Refresh();
	}
	
	virtual void OnMouseDrag(int x, int y) {
		if(mouse_down) {
			Point _d(start-Point(x,y));
			Vec3d dv = Vec3d(start.x,start.y,0) - Vec3d(x,y,50);
			d0v = Vec3d(start.x,start.y,0) - Vec3d(x,y,0);
			switch (state) {
				case STATE_ROTATING_XY:
					faces[currentFace].face_z_rot = Vec3d(dv * (1.0 / norm(dv))).dot(Vec3d(1,0,0)) * -RAD_TO_DEG;
					faces[currentFace].face_x_rot = Vec3d(dv * (1.0 / norm(dv))).dot(Vec3d(0,1,0)) * -RAD_TO_DEG;
					break;
				case STATE_ROTATING_Z:
					faces[currentFace].face_y_rot = atan2(d0v[0], d0v[1]) * -RAD_TO_DEG;
					break;
				case STATE_SCALING:
					faces[currentFace].face_scale += norm(last-Point(x,y)) * SIGN(last.x-x);
					cout << "scale " << faces[currentFace].face_scale << endl;
					break;
				case STATE_MOVING:
					faces[currentFace].face_pos.x += (last.x - x);
					faces[currentFace].face_pos.y += (last.y - y);
					glLightfv(GL_LIGHT0, GL_POSITION, Vec4f(faces[currentFace].face_pos.x + 10, faces[currentFace].face_pos.y, -2, 1.0).val);
					cout << "face_pos " << faces[currentFace].face_pos <<endl;
					break;
				default:
					break;
			}
			last.x = x; last.y = y;
			doRelighting = true;
			glowWindow->Refresh();
		}
	}
	
	
	void OnKeyboard(Glow::KeyCode key, int x, int y, Glow::Modifiers modifiers) {
		cout << "key " << key << endl;
		if (key == 's' || key == 'S') {
			state = STATE_SCALING;
		} else if (key == 'r' || key == 'R') {
			state = STATE_ROTATING_XY;
		} else if (key == 't' || key == 'T') {
			state = STATE_ROTATING_Z;
		} else if (key == 'm' || key == 'M') {
			state = STATE_MOVING;
		} else if (key == 'q' || key == 'Q') {
			//glutLeaveMainLoop();
			glowWindow->Close();
		} else if (key == 'o' || key == 'O') {
			glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, &(Vec4f(.8,.8,.8,(face_opaque)?.6:1.0)[0]));
			face_opaque = !face_opaque;
		} else if (key == 'd' || key == 'D') {
			showFace = !showFace;
		} else if (key == 'h' || key == 'H') {
			useShaders = !useShaders;
		} else if (key == ' ') {
			doRelighting = true;
//		} else if (key == '2') {
//			switchToFace(2);
//		} else if (key == '1') {
//			switchToFace(1);
		} else if (key == 'v') {
			faces[0].saveToFile("face0_orientation.yml");
			faces[1].saveToFile("face1_orientation.yml");
		} else if (key == 'f') {
			faces[0].readFromFile("face0_orientation.yml");
			faces[1].readFromFile("face1_orientation.yml");
			glLightfv(GL_LIGHT0, GL_POSITION, Vec4f(faces[currentFace].face_pos.x + 10, faces[currentFace].face_pos.y, -2, 1.0).val);
		} else if (key == 'c' || key == 'C') {
			align_sha[currentFace]->_debug = true;
			align_sha[1]->renderWithCoefficients(align_sha[0]->getLightingCoefficients());
			align_sha[0]->renderWithCoefficients(align_sha[1]->getLightingCoefficients());
		} else {
			state = STATE_IDLE;
		}
		glowWindow->Refresh();
	}
	void switchToFace(int n) {
		currentFace = n-1;
	}
	
//	int main(int argc, char** argv) {
//		Mat img = imread(IMAGE_FILENAME1);
//		
//		glutInit(&argc, argv);
//		glutInitWindowSize(640, 640);
//		glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
//		glutwin = glutCreateWindow("simple");
//		glutDisplayFunc(align_display);
//		glutKeyboardFunc(align_keyboard);
//		glutIdleFunc(align_display);
//		//	glutPassiveMotionFunc(mouse);
//		glutMouseFunc(align_processMouse);
//		glutMotionFunc(align_mouse);
//		align_init();
//		
//		
//		glutMainLoop();
//		
//	}	
};



