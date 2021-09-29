#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string.h>
#include <stdlib.h>
#include <string>
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <sstream>
#include <cmath>
#include <vector>
#include <algorithm>

#include "file_utils.h"
#include "math_utils.h"

using namespace std;

/********************************************************************/
/*   Variables */

const char * theProgramTitle = "Assignment1";
int theWindowWidth = 700, theWindowHeight = 700;
int theWindowPositionX = 40, theWindowPositionY = 40;
bool isFullScreen = false;
bool isAnimating = true;
float rotation = 0.0f;
GLuint VBO, VAO;
GLuint vao[2],vbo[2],ibo[2];
GLsizei l, r;
GLint gWorldLocation, translateLocation;

/* Constants */
const int ANIMATION_DELAY = 20; /* milliseconds between rendering */
const char* pVSFileName = "shader.vs";
const char* pFSFileName = "shader.fs";

/********************************************************************
  Utility functions
 */

/* post: compute frames per second and display in window's title bar */
void computeFPS() {
	static int frameCount = 0;
	static int lastFrameTime = 0;
	static char * title = NULL;
	int currentTime;

	if (!title)
		title = (char*) malloc((strlen(theProgramTitle) + 20) * sizeof (char));
	frameCount++;
	currentTime = glutGet((GLenum) (GLUT_ELAPSED_TIME));
	if (currentTime - lastFrameTime > 1000) {
		sprintf(title, "%s [ FPS: %4.2f ]",
			theProgramTitle,
			frameCount * 1000.0 / (currentTime - lastFrameTime));
		glutSetWindowTitle(title);
		lastFrameTime = currentTime;
		frameCount = 0;
	}
}

static float normaliseVertexAttribute(float x, float min, float max) {
	return (2*((x-min)/(max-min))-1.0);
} 

float t;
static bool HelperToFindIntersectionPoint ( Vector3f X, Vector3f Y ) {
	if ( Y.x - X.x ) {
		t = X.x/(X.x-Y.x);
		return true;
	}
	else return false;
}

static float EuclideanDistance ( Vector3f X, Vector3f Y ) {
	return sqrt(pow(X.x-Y.x,2)+pow(X.y-Y.y,2)+pow(X.z-Y.z,2));
}

static bool ValidVertex( Vector3f X, Vector3f Y, Vector3f P) {
	float distXY = EuclideanDistance(X,Y);
	float distXP = EuclideanDistance(X,P);
	float distYP = EuclideanDistance(Y,P);

	if ( (distXP > distXY) || (distYP > distXY) ) 
		return false;
	return true;
}


static int decider( Vector3f A, Vector3f B, Vector3f C) {
	if ( A.x <= 0.0f && B.x <= 0.0f && C.x <= 0.0f ) 
		return 1;
	else if ( A.x >= 0.0f && B.x >= 0.0f && C.x >= 0.0f ) 
		return 2;
	else {
		return 3;
	}
}

static void sliceModel(Vector3f vertices[], int vcount, unsigned int faces[], int fcount) {
	vector<Vector3f> L,R;

	unsigned int a,b,c;
	Vector3f A,B,C;
	for ( int i=0; i<fcount; i+=3 ) {
		a = faces[i]; b = faces[i+1]; c = faces[i+2]; // indices of triangle
		A = vertices[a]; B = vertices[b]; C = vertices[c]; // vertices of triangle
		//A.Print();  B.Print();  C.Print(); 
		switch ( decider(A,B,C)) {
			case 1:
				A.x = A.x-0.5f; B.x = B.x-0.5f; C.x = C.x-0.5f; 
				L.push_back(A);
				L.push_back(B);
				L.push_back(C);
				break;
			case 2:
				A.x = A.x+0.5f; B.x = B.x+0.5f; C.x = C.x+0.5f; 
				R.push_back(A);
				R.push_back(B);
				R.push_back(C);
				break;
			case 3:
				float x,y,z;
				bool p=false,q=false,s=false;
				Vector3f P,Q,S;
				if (HelperToFindIntersectionPoint(A,B)) {
					x = A.x + t*(B.x-A.x);
					y = A.y + t*(B.y-A.y);
					z = A.z + t*(B.z-A.z);
					P = Vector3f(x,y,z);
					if (ValidVertex(A,B,P)){
						p=true;
					}
				}
				if (HelperToFindIntersectionPoint(B,C)) {
					x = B.x + t*(C.x-B.x);
					y = B.y + t*(C.y-B.y);
					z = B.z + t*(C.z-B.z);
					Q = Vector3f(x,y,z);
					if (ValidVertex(B,C,Q)){
						q=true;
					}
				}
				if (HelperToFindIntersectionPoint(C,A)) {
					x = C.x + t*(A.x-C.x);
					y = C.y + t*(A.y-C.y);
					z = C.z + t*(A.z-C.z);
					S = Vector3f(x,y,z);
					if (ValidVertex(C,A,S)){
						s=true;
					}
				}

				if (p&&q) {
					if ( A.x <= 0.0f ) {
						A.x-=0.5f; P.x-=0.5f; Q.x-=0.5f; C.x-=0.5f;
						L.push_back(A); L.push_back(P); L.push_back(C);
						L.push_back(P); L.push_back(Q); L.push_back(C);
						P.x+=1.0f; Q.x+=1.0f; B.x+=0.5f;
						R.push_back(P); R.push_back(Q); R.push_back(B);
					}
					else {
						A.x+=0.5f; P.x+=0.5f; Q.x+=0.5f; C.x+=0.5f;
						R.push_back(A); R.push_back(P); R.push_back(C);
						R.push_back(P); R.push_back(Q); R.push_back(C);
						P.x-=1.0f; Q.x-=1.0f; B.x-=0.5f;
						L.push_back(P); L.push_back(Q); L.push_back(B);
					}
				}
				else if (q&&s) {
					if ( A.x <= 0.0f ) {
						A.x-=0.5f; S.x-=0.5f; Q.x-=0.5f; B.x-=0.5f;
						L.push_back(A); L.push_back(S); L.push_back(B);
						L.push_back(S); L.push_back(Q); L.push_back(B);
						S.x+=1.0f; Q.x+=1.0f; C.x+=0.5f;
						R.push_back(S); R.push_back(Q); R.push_back(C);
					}
					else {
						A.x+=0.5f; S.x+=0.5f; Q.x+=0.5f; B.x+=0.5f;
						R.push_back(A); R.push_back(S); R.push_back(B);
						R.push_back(S); R.push_back(Q); R.push_back(B);
						S.x-=1.0f; Q.x-=1.0f; C.x-=0.5f;
						L.push_back(S); L.push_back(Q); L.push_back(C);
					}

				}
				else if (p&&s) {
					if ( B.x <= 0.0f ) {
						B.x-=0.5f; S.x-=0.5f; P.x-=0.5f; C.x-=0.5f;
						L.push_back(B); L.push_back(P); L.push_back(C);
						L.push_back(P); L.push_back(S); L.push_back(C);
						S.x+=1.0f; P.x+=1.0f; A.x+=0.5f;
						R.push_back(S); R.push_back(P); R.push_back(A);
					}
					else {
						B.x+=0.5f; S.x+=0.5f; P.x+=0.5f; C.x+=0.5f;
						R.push_back(B); R.push_back(P); R.push_back(S);
						R.push_back(B); R.push_back(C); R.push_back(S);
						S.x-=1.0f; P.x-=1.0f; A.x-=0.5f;
						L.push_back(S); L.push_back(P); L.push_back(A);
					}
				}

		}
	}

	l = L.size();
	r = R.size();

	glGenVertexArrays(2, vao);
	glGenBuffers(2, vbo);

	// Left
	glBindVertexArray(vao[0]);
	glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
	glBufferData(GL_ARRAY_BUFFER, L.size()*3*sizeof(float), &L[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE, 0, 0);
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER,0);

	//Right
	glBindVertexArray(vao[1]);
	glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
	glBufferData(GL_ARRAY_BUFFER, R.size()*3*sizeof(float), &R[0], GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE, 0, 0);
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER,0);
}

static void CreateVertexBuffer() {
	const char* vfile = "m415/ver.off";
	const char* ffile = "m415/faces.off";

	float xmin = 0.025, ymin = 0.025, zmin = 0.025, xmax = 0.695588, ymax = 0.460882, zmax = 0.975;

	Vector3f vertices[508];
	int j=0;
	std::ifstream file(vfile);
	if (file.is_open()) {
	    std::string line;
	    while (getline(file, line)) {
	        stringstream ss;
	    	ss << line;
	    	//printf("%s\n", line.c_str());
	    	string temp;
	    	float arr[3];
	    	int i=0;
	    	while(!ss.eof()){
	    		ss >> temp;
	    		if(stringstream(temp) >> arr[i])
	    			i++;
	    		temp="";
	    	}
	    	arr[0] = normaliseVertexAttribute(arr[0],xmin,xmax);
	    	arr[1] = normaliseVertexAttribute(arr[1],ymin,ymax);
	    	arr[2] = normaliseVertexAttribute(arr[2],zmin,zmax);
	    	vertices[j++] = Vector3f(arr[0],arr[1],arr[2]);
   		 }
 	   file.close();
	}

	unsigned int faces[2898];
	j=0;
	
	file.open(ffile);
	if (file.is_open()) {
	    std::string line;
	    while (getline(file, line)) {
	        stringstream ss;
	    	ss << line;
	    	//printf("%s\n", line.c_str());
	    	string temp;
	    	int arr[4];
	    	int i=0;
	    	while(!ss.eof()){
	    		ss >> temp;
	    		if(stringstream(temp) >> arr[i])
	    			i++;
	    		temp="";
	    	}
	    	//printf("%d %d %d \n",arr[1], arr[2], arr[3]);
	    	for (i=1;i<=3;i++) {
	    		faces[j++] = arr[i];
	    	}
	    }
	    
 	   file.close();
	}

	sliceModel(vertices, 508, faces, 2898);
}

static void AddShader(GLuint ShaderProgram, const char* pShaderText, GLenum ShaderType) {
	GLuint ShaderObj = glCreateShader(ShaderType);

	if (ShaderObj == 0) {
		fprintf(stderr, "Error creating shader type %d\n", ShaderType);
		exit(0);
	}

	const GLchar * p[1];
	p[0] = pShaderText;
	GLint Lengths[1];
	Lengths[0] = strlen(pShaderText);
	glShaderSource(ShaderObj, 1, p, Lengths);
	glCompileShader(ShaderObj);
	GLint success;
	glGetShaderiv(ShaderObj, GL_COMPILE_STATUS, &success);
	if (!success) {
		GLchar InfoLog[1024];
		glGetShaderInfoLog(ShaderObj, 1024, NULL, InfoLog);
		fprintf(stderr, "Error compiling shader type %d: '%s'\n", ShaderType, InfoLog);
		exit(1);
	}

	glAttachShader(ShaderProgram, ShaderObj);
}


static void CompileShaders() {
	GLuint ShaderProgram = glCreateProgram();

	if (ShaderProgram == 0) {
		fprintf(stderr, "Error creating shader program\n");
		exit(1);
	}

	string vs, fs;

	if (!ReadFile(pVSFileName, vs)) {
		exit(1);
	}

	if (!ReadFile(pFSFileName, fs)) {
		exit(1);
	}

	AddShader(ShaderProgram, vs.c_str(), GL_VERTEX_SHADER);
	AddShader(ShaderProgram, fs.c_str(), GL_FRAGMENT_SHADER);

	GLint Success = 0;
	GLchar ErrorLog[1024] = {0};

	glLinkProgram(ShaderProgram);
	glGetProgramiv(ShaderProgram, GL_LINK_STATUS, &Success);
	if (Success == 0) {
		glGetProgramInfoLog(ShaderProgram, sizeof (ErrorLog), NULL, ErrorLog);
		fprintf(stderr, "Error linking shader program: '%s'\n", ErrorLog);
		exit(1);
	}

	glValidateProgram(ShaderProgram);
	glGetProgramiv(ShaderProgram, GL_VALIDATE_STATUS, &Success);
	if (!Success) {
		glGetProgramInfoLog(ShaderProgram, sizeof (ErrorLog), NULL, ErrorLog);
		fprintf(stderr, "Invalid shader program: '%s'\n", ErrorLog);
		exit(1);
	}

	glUseProgram(ShaderProgram);
	gWorldLocation = glGetUniformLocation(ShaderProgram, "gWorld");
	translateLocation = glGetUniformLocation(ShaderProgram, "translate");
}

/********************************************************************
 Callback Functions
 These functions are registered with the glut window and called 
 when certain events occur.
 */

void onInit(int argc, char * argv[])
/* pre:  glut window has been initialized
   post: model has been initialized */ {
	/* by default the back ground color is black */
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	CreateVertexBuffer();
	CompileShaders();

	/* set to draw in window based on depth  */
	glEnable(GL_DEPTH_TEST); 
}

static void onDisplay() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	Matrix4f World;

	World.m[0][0] = cosf(3.142); World.m[0][1] = 0.0f; World.m[0][2] = sinf(3.142); World.m[0][3] = 0.0f;
	World.m[1][0] = 0.0f; World.m[1][1] = 1.0f;  World.m[1][2] = 0.0f; World.m[1][3] = 0.0f;
	World.m[2][0] = -sinf(3.142);        World.m[2][1] = 0.0f;         World.m[2][2] = cosf(3.142); World.m[2][3] = 0.0f;
	World.m[3][0] = 0.0f;        World.m[3][1] = 0.0f;         World.m[3][2] = 0.0f; World.m[3][3] = 1.0f;

	glUniformMatrix4fv(gWorldLocation, 1, GL_TRUE, &World.m[0][0]);

	//Left
	glBindVertexArray(vao[0]);
	glDrawArrays(GL_TRIANGLES, 0, l);
	glBindVertexArray(0);
	//Right
	glBindVertexArray(vao[1]);
	glDrawArrays(GL_TRIANGLES, 0, r);
	glBindVertexArray(0);

	/* check for any errors when rendering */
	GLenum errorCode = glGetError();
	if (errorCode == GL_NO_ERROR) {
		/* double-buffering - swap the back and front buffers */
		glutSwapBuffers();
	} else {
		fprintf(stderr, "OpenGL rendering error %d\n", errorCode);
	}
}

/* pre:  glut window has been resized
 */
static void onReshape(int width, int height) {
	glViewport(0, 0, width, height);
	if (!isFullScreen) {
		theWindowWidth = width;
		theWindowHeight = height;
	}
	// update scene based on new aspect ratio....
}

/* pre:  glut window is not doing anything else
   post: scene is updated and re-rendered if necessary */
static void onIdle() {
	static int oldTime = 0;
	if (isAnimating) {
		int currentTime = glutGet((GLenum) (GLUT_ELAPSED_TIME));
		/* Ensures fairly constant framerate */
		if (currentTime - oldTime > ANIMATION_DELAY) {
			// do animation....
			rotation += 0.005;

			oldTime = currentTime;
			/* compute the frame rate */
			computeFPS();
			/* notify window it has to be repainted */
			glutPostRedisplay();
		}
	}
}

/* pre:  mouse is dragged (i.e., moved while button is pressed) within glut window
   post: scene is updated and re-rendered  */
static void onMouseMotion(int x, int y) {
	/* notify window that it has to be re-rendered */
	glutPostRedisplay();
}

/* pre:  mouse button has been pressed while within glut window
   post: scene is updated and re-rendered */
static void onMouseButtonPress(int button, int state, int x, int y) {
	if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
		// Left button pressed
	}
	if (button == GLUT_LEFT_BUTTON && state == GLUT_UP){
		// Left button un pressed
	}
	/* notify window that it has to be re-rendered */
	glutPostRedisplay();
}

/* pre:  key has been pressed
   post: scene is updated and re-rendered */
static void onAlphaNumericKeyPress(unsigned char key, int x, int y) {
	switch (key) {
			/* toggle animation running */
		case 'a':
			isAnimating = !isAnimating;
			break;
			/* reset */
		case 'r':
			rotation = 0;
			break;
			/* quit! */
		case 'Q':
		case 'q':
		case 27:
			exit(0);
	}

	/* notify window that it has to be re-rendered */
	glutPostRedisplay();
}

/* pre:  arrow or function key has been pressed
   post: scene is updated and re-rendered */
static void onSpecialKeyPress(int key, int x, int y) {
	/* please do not change function of these keys */
	switch (key) {
			/* toggle full screen mode */
		case GLUT_KEY_F1:
			isFullScreen = !isFullScreen;
			if (isFullScreen) {
				theWindowPositionX = glutGet((GLenum) (GLUT_WINDOW_X));
				theWindowPositionY = glutGet((GLenum) (GLUT_WINDOW_Y));
				glutFullScreen();
			} else {
				glutReshapeWindow(theWindowWidth, theWindowHeight);
				glutPositionWindow(theWindowPositionX, theWindowPositionY);
			}
			break;
	}

	/* notify window that it has to be re-rendered */
	glutPostRedisplay();
}

/* pre:  glut window has just been iconified or restored 
   post: if window is visible, animate model, otherwise don't bother */
static void onVisible(int state) {
	if (state == GLUT_VISIBLE) {
		/* tell glut to show model again */
		glutIdleFunc(onIdle);
	} else {
		glutIdleFunc(NULL);
	}
}

static void InitializeGlutCallbacks() {
	/* tell glut how to display model */
	glutDisplayFunc(onDisplay);
	/* tell glutwhat to do when it would otherwise be idle */
	glutIdleFunc(onIdle);
	/* tell glut how to respond to changes in window size */
	glutReshapeFunc(onReshape);
	/* tell glut how to handle changes in window visibility */
	glutVisibilityFunc(onVisible);
	/* tell glut how to handle key presses */
	glutKeyboardFunc(onAlphaNumericKeyPress);
	glutSpecialFunc(onSpecialKeyPress);
	/* tell glut how to handle the mouse */
	glutMotionFunc(onMouseMotion);
	glutMouseFunc(onMouseButtonPress);
}

int main(int argc, char** argv) {
	glutInit(&argc, argv);

	/* request initial window size and position on the screen */
	glutInitWindowSize(theWindowWidth, theWindowHeight);
	glutInitWindowPosition(theWindowPositionX, theWindowPositionY);
	/* request full color with double buffering and depth-based rendering */
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_DEPTH | GLUT_RGBA);
	/* create window whose title is the name of the executable */
	glutCreateWindow(theProgramTitle);

	InitializeGlutCallbacks();

	// Must be done after glut is initialized!
	GLenum res = glewInit();
	if (res != GLEW_OK) {
		fprintf(stderr, "Error: '%s'\n", glewGetErrorString(res));
		return 1;
	}

	printf("GL version: %s\n", glGetString(GL_VERSION));

	/* initialize model */
	onInit(argc, argv);

	/* give control over to glut to handle rendering and interaction  */
	glutMainLoop();

	/* program should never get here */

	return 0;
}

