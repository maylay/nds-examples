/********************************************************************************************
 * 		Nehe lesson 10 modification which uses the touch screen to control the camera and dpad
 *		to move the player.

 * 		Author: revo																		*
 *		Updated by revo (from 10b) - added camera moving by touching touch screen
 *
 *      $Log: not supported by cvs2svn $
 *      Revision 1.4  2005/09/05 00:32:20  wntrmute
 *      removed references to IPC struct
 *      replaced with API functions
 *
 *      Revision 1.3  2005/08/31 03:02:39  wntrmute
 *      updated for new stdio support
 *
 *      Revision 1.2  2005/08/11 15:51:33  desktopman
 *      Added strafe
 *      Added mirror controls for lefties
 *      Added filtering of small changes to prevent flickering
 *      Slowed down movement a bit
 *      Now destroys the pcx struct after use
 *
 *      Revision 1.1  2005/07/31 18:25:36  dovoto
 *      Added the touch look demo by revo to demonstrate camera control by touch pad in a 3D scene
 *
 ********************************************************************************************/

// include your ndslib
#include <nds.h>
#include <malloc.h>
#include <stdio.h>

//needed to load pcx files
#include <nds/arm9/image.h>

#include <nds/arm9/trig_lut.h>

#include "Mud_pcx.h"
#include "World_txt.h"


int DrawGLScene();

int heading;
f32 xpos;
f32 zpos;

int	yrot;				// Y Rotation
f32 walkbias = 0;
int walkbiasangle = 0;
int lookupdown = 0;


int	texture[0];			// Storage For 1 Textures (only going to use 1 on the DS for this demo)

typedef struct tagVERTEX
{
	v16 x, y, z;
	t16 u, v;
} VERTEX;

typedef struct tagTRIANGLE
{
	VERTEX vertex[3];
} TRIANGLE;

typedef struct tagSECTOR
{
	int numtriangles;
	TRIANGLE* triangle;
} SECTOR;

SECTOR sector1;				// Our Model Goes Here:



 u8* file = (u8*)World_txt;

 void myGetStr(char* buff, int size)
{
	*buff = *file++;

	while( (*buff != '\n') && (*buff != 0xD))
	{
		buff++;
		*buff = *file++;
	}

	buff[0] = '\n';
	buff[1] = 0;

}


void readstr(char *string)
{
	do
	{
		myGetStr(string, 255);
	} while ((string[0] == '/') || (string[0] == '\n' ));
	return;
}

void SetupWorld()
{
	float x, y, z;
	float u, v;
	int numtriangles;
	char oneline[255];

	readstr(oneline);
	sscanf(oneline, "NUMPOLLIES %d\n", &numtriangles);

	sector1.triangle = (TRIANGLE*)malloc(numtriangles*sizeof(TRIANGLE));
	sector1.numtriangles = numtriangles;

	for (int loop = 0; loop < numtriangles; loop++)
	{
		for (int vert = 0; vert < 3; vert++)
		{
			readstr(oneline);
			sscanf(oneline, "%f %f %f %f %f", &x, &y, &z, &u, &v);
			sector1.triangle[loop].vertex[vert].x = floatov16(x);
			sector1.triangle[loop].vertex[vert].y = floatov16(y);
			sector1.triangle[loop].vertex[vert].z = floatov16(z);
			sector1.triangle[loop].vertex[vert].u = floatot16(u*128);
			sector1.triangle[loop].vertex[vert].v = floatot16(v*128);
		}
	}

	return;
}
int LoadGLTextures()									// Load PCX files And Convert To Textures
{
	sImage pcx;                //////////////(NEW) and different from nehe.

	//load our texture
	loadPCX((u8*)Mud_pcx, &pcx);

	image8to16(&pcx);

	glGenTextures(1, &texture[0]);
	glBindTexture(0, texture[0]);
	glTexImage2D(0, 0, GL_RGB, TEXTURE_SIZE_128 , TEXTURE_SIZE_128, 0, TEXGEN_TEXCOORD | GL_TEXTURE_WRAP_S | GL_TEXTURE_WRAP_T, pcx.data8);

	imageDestroy(&pcx);

	return TRUE;
}

int main()
{


	// Turn on everything
	powerON(POWER_ALL);

	// Setup the Main screen for 3D
	videoSetMode(MODE_0_3D);
	vramSetBankA(VRAM_A_TEXTURE);                        //NEW  must set up some memory for textures

	// IRQ basic setup
	irqInit();
	irqSet(IRQ_VBLANK, 0);

	// Set our viewport to be the same size as the screen
	glViewPort(0,0,255,191);

	// Specify the Clear Color and Depth
	glClearColor(0,0,0);
	glClearDepth(0x7FFF);
	LoadGLTextures();
	SetupWorld();

	touchPosition	thisXY;
	touchPosition	lastXY = { 0,0,0,0 };		

	while (1)
	{
		//these little button functions are pretty handy
		scanKeys();

		if (keysHeld() & (KEY_LEFT|KEY_Y))
		{
			xpos -= SIN[(heading+128)& LUT_MASK] >> 5;
			zpos += COS[(heading+128)& LUT_MASK] >> 5;
		}
		if (keysHeld() & (KEY_RIGHT|KEY_A))
		{
			xpos += SIN[(heading+128)& LUT_MASK] >> 5;
			zpos -= COS[(heading+128)& LUT_MASK] >> 5;
		}
		if (keysHeld() & (KEY_DOWN|KEY_B))
		{

			xpos -= SIN[heading & LUT_MASK]>>5;
			zpos += COS[heading & LUT_MASK]>>5;

			walkbiasangle+= 10;

			walkbias = SIN[walkbiasangle & LUT_MASK]>>4;
		}
		if (keysHeld() & (KEY_UP|KEY_X))
		{
			xpos += SIN[heading& LUT_MASK] >> 5;
			zpos -= COS[heading& LUT_MASK] >> 5;

			if (walkbiasangle <= 0)
			{
				walkbiasangle = LUT_SIZE;
			}
			else
			{
				walkbiasangle-= 10;
			}
			walkbias = SIN[walkbiasangle & LUT_MASK]>>4;
		}

		// Camera rotation by touch screen

		if (keysHeld() & KEY_TOUCH)
		{
			thisXY = touchReadXY();

			int16 dx = thisXY.px - lastXY.px;
			int16 dy = thisXY.py - lastXY.py;

			// filtering measurement errors
			if (dx<20 && dx>-20 && dy<20 && dy>-20)
			{
				if(dx>-3&&dx<3)
					dx=0;

				if(dy>-2&&dy<2)
					dy=0;

					lookupdown -= dy;

					heading += dx;
					yrot = heading;
			}

			lastXY = thisXY;
		}


		// Reset the screen and setup the view
		glReset();
		gluPerspective(35, 256.0 / 192.0, 0.1, 100);
		glColor3f(1,1,1);

		glLight(0, RGB15(31,31,31) , 0,	floatov10(-1.0), 0);

		glPushMatrix();

		glMatrixMode(GL_TEXTURE);
		glIdentity();

		glMatrixMode(GL_MODELVIEW);

		//need to set up some material properties since DS does not have them set by default
		glMaterialf(GL_AMBIENT, RGB15(16,16,16));
		glMaterialf(GL_DIFFUSE, RGB15(16,16,16));
		glMaterialf(GL_SPECULAR, BIT(15) | RGB15(8,8,8));
		glMaterialf(GL_EMISSION, RGB15(16,16,16));

		//ds uses a table for shinyness..this generates a half-ass one
		glMaterialShinyness();


		//ds specific, several attributes can be set here
		glPolyFmt(POLY_ALPHA(31) | POLY_CULL_NONE | POLY_FORMAT_LIGHT0);

		// Set the current matrix to be the model matrix
		glMatrixMode(GL_MODELVIEW);

		//Push our original Matrix onto the stack (save state)
		glPushMatrix();

		DrawGLScene();

		// Pop our Matrix from the stack (restore state)
		glPopMatrix(1);

		// flush to screen
		glFlush();

	}

	return 0;
}

int DrawGLScene()											// Here's Where We Do All The Drawing
{
											// Reset The View

	v16 x_m, y_m, z_m;
	t16 u_m, v_m;
	f32 xtrans = -xpos;
	f32 ztrans = -zpos;
	f32 ytrans = -walkbias-(1<<10);
	int sceneroty = LUT_SIZE - yrot;

	glLoadIdentity();

	int numtriangles;

	glRotatef32i(lookupdown,(1<<12),0,0);
	glRotatef32i(sceneroty,0,(1<<12),0);

	glTranslate3f32(xtrans, ytrans, ztrans);
	glBindTexture(GL_TEXTURE_2D, texture[0]);

	numtriangles = sector1.numtriangles;


	// Process Each Triangle
	for (int loop_m = 0; loop_m < numtriangles; loop_m++)
	{
		glBegin(GL_TRIANGLES);
			glNormal(NORMAL_PACK( 0, 0, 1<<10));
			x_m = sector1.triangle[loop_m].vertex[0].x;
			y_m = sector1.triangle[loop_m].vertex[0].y;
			z_m = sector1.triangle[loop_m].vertex[0].z;
			u_m = sector1.triangle[loop_m].vertex[0].u;
			v_m = sector1.triangle[loop_m].vertex[0].v;
			glTexCoord2t16(u_m,v_m); glVertex3v16(x_m,y_m,z_m);

			x_m = sector1.triangle[loop_m].vertex[1].x;
			y_m = sector1.triangle[loop_m].vertex[1].y;
			z_m = sector1.triangle[loop_m].vertex[1].z;
			u_m = sector1.triangle[loop_m].vertex[1].u;
			v_m = sector1.triangle[loop_m].vertex[1].v;
			glTexCoord2t16(u_m,v_m); glVertex3v16(x_m,y_m,z_m);

			x_m = sector1.triangle[loop_m].vertex[2].x;
			y_m = sector1.triangle[loop_m].vertex[2].y;
			z_m = sector1.triangle[loop_m].vertex[2].z;
			u_m = sector1.triangle[loop_m].vertex[2].u;
			v_m = sector1.triangle[loop_m].vertex[2].v;
			glTexCoord2t16(u_m,v_m); glVertex3v16(x_m,y_m,z_m);
		glEnd();
	}
	return TRUE;										// Everything Went OK

}