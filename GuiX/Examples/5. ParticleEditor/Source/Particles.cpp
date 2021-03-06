#include <GuiX/Config.h>

#include <math.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <Gl/gl.h>

#include <GuiX/Common.h>
#include <GuiX/Draw.h>

#include "Particles.h"
#include "Matrix.h"

namespace guix {
namespace particles {

inline float Randf()
{
	return (1.f / RAND_MAX) * (float)rand();
}

inline float Randf(float min, float max) 
{
	return min + (1.f / RAND_MAX) * (max - min) * (float)rand();
}

// ------------- QUAD

static float quadNormals[] = {
	0,0,-1,  0,0,-1,  0,0,-1,  0,0,-1,
	0,0,+1,	 0,0,+1,  0,0,+1,  0,0,+1,
	-1,0,0,	 -1,0,0,  -1,0,0,  -1,0,0,
	+1,0,0,	 +1,0,0,  +1,0,0,  +1,0,0,
	0,-1,0,	 0,-1,0,  0,-1,0,  0,-1,0,
	0,+1,0,	 0,+1,0,  0,+1,0  ,0,+1,0,
};

static float quadVerts[] = {
	-.5f,+.5f,-.5f,   -.5f,-.5f,-.5f,   +.5f,-.5f,-.5f,   +.5f,+.5f,-.5f,
	+.5f,-.5f,+.5f,   -.5f,-.5f,+.5f,   -.5f,+.5f,+.5f,   +.5f,+.5f,+.5f,
	-.5f,-.5f,+.5f,   -.5f,-.5f,-.5f,   -.5f,+.5f,-.5f,   -.5f,+.5f,+.5f,
	+.5f,+.5f,-.5f,   +.5f,-.5f,-.5f,   +.5f,-.5f,+.5f,   +.5f,+.5f,+.5f,
	+.5f,-.5f,-.5f,   -.5f,-.5f,-.5f,   -.5f,-.5f,+.5f,   +.5f,-.5f,+.5f,
	-.5f,+.5f,+.5f,   -.5f,+.5f,-.5f,   +.5f,+.5f,-.5f,   +.5f,+.5f,+.5f,
};

static int quadVCount = 24;

// ------------- TRIANGLE

static float squareNormals[] = {
	0,0,+1,   0,0,+1,   0,0,+1,   0,0,+1,
	0,0,-1,   0,0,-1,   0,0,-1,   0,0,-1,
};

static float squareVerts[] = {
	-.5f,-.5f,0,   -.5f,+.5f,0,   +.5f,+.5f,0,  +.5f,-.5f,0,
	-.5f,+.5f,0,   -.5f,-.5f,0,   +.5f,-.5f,0,  +.5f,+.5f,0,
};

static int squareVCount = 8;

// ===================================================================================
// GxParticleData
// ===================================================================================

GxParticleData::GxParticleData()
{
	for(int i=0; i<PD_COUNT; ++i)
		data[i] = NULL;
}

void GxParticleData::Clear()
{
	for(int i=0; i<PD_COUNT; ++i)
	{
		GxFree(data[i]);
		data[i] = NULL;
	}
}

void GxParticleData::Resize(int size)
{
	for(int i=0; i<PD_COUNT; ++i)
		data[i] = GxRealloc(data[i], size);
}

// ===================================================================================
// GxParticleProperty
// ===================================================================================

void GxParticleProperty::OptimizeFlags()
{
	if(value == PV_RANDOM && a == b)
		value = PV_CONSTANT;

	if(value == PV_CONSTANT && a == 0)
		value = PV_ZERO;

	if(delta == PD_LERP_TO_RANDOM && c == d)
		delta = PD_LERP_TO_CONSTANT;

	if(delta == PD_LERP_TO_CONSTANT && value == PV_CONSTANT && a == c)
		delta = PD_ZERO;

	if(delta == PD_RANDOM && c == d)
		delta = PD_CONSTANT;

	if(delta == PD_CONSTANT && c == 0)
		delta = PD_ZERO;
}

// ===================================================================================
// GxParticleType
// ===================================================================================

GxParticleType::GxParticleType()
	:life          (4.f)
	,r             (1.0f, 0.5f)
	,g             (0.5f, 0.3f)
	,b             (0.2f, 0.8f)
	,vel           (8.f)
	,rotX          (0.f, 360, 0.f, 360)
	,rotY          (0.f, 360, 0.f, 360)
	,rotZ          (0.f, 360, 0.f, 360)
	,size          (1.f, 0.f, 0.5f, 0.0f)
	,spawnSpread   (0.75f)
	,externalForce (0.f, -2.f, 0.f)
	,blendMode     (0)
{
}

// ===================================================================================
// GxParticleEmitter
// ===================================================================================

GxParticleEmitter::GxParticleEmitter()
	:myCount(0)
	,myReserved(0)
{
}

GxParticleEmitter::~GxParticleEmitter()
{
}

void GxParticleEmitter::SetType(const GxParticleType& type)
{
	myType = type;
	myData.Clear();
}

void GxParticleEmitter::InitProperty(const GxParticleProperty& prop, float* values, float* deltas, float* life, int begin, int count)
{
	if(values)
	{
		const float a = prop.a, b = prop.b;
		switch(prop.value)
		{
		case PV_ZERO:
			for(int i=begin; i<count; ++i) values[i] = 0; break;
		case PV_CONSTANT:
			for(int i=begin; i<count; ++i) values[i] = a; break;
		case PV_RANDOM:
			for(int i=begin; i<count; ++i) values[i] = Randf(a, b);	break;
		};
	}
	if(deltas)
	{
		const float c = prop.c, d = prop.d;
		switch(prop.delta)
		{
		case PD_ZERO:
			for(int i=begin; i<count; ++i) deltas[i] = 0; break;
		case PD_CONSTANT:
			for(int i=begin; i<count; ++i) deltas[i] = c; break;
		case PD_RANDOM:
			for(int i=begin; i<count; ++i) deltas[i] = Randf(c, d);	break;
		case PD_LERP_TO_CONSTANT:
			for(int i=begin; i<count; ++i) deltas[i] = (c - values[i]) / life[i]; break;
		case PD_LERP_TO_RANDOM:
			for(int i=begin; i<count; ++i) deltas[i] = (Randf(c, d) - values[i]) / life[i];	break;
		};
	}
}

void GxParticleEmitter::TickProperty(const GxParticleProperty& prop, float* values, float* deltas, float dt)
{
	if(values && deltas)
	{
		for(int i=0; i<myCount; ++i) 
			values[i] += deltas[i] * dt;
	}
	else if(values && prop.delta != PD_ZERO)
	{
		const float delta = prop.c * dt;
		for(int i=0; i<myCount; ++i) 
			values[i] += delta;
	}
}

void GxParticleEmitter::Spawn(const GxVec3f& spawnPos, const GxVec3f& spawnDir, int amount)
{
	// GxClamp amount to MAX_PARTICLE_COUNT
	if(myCount + amount > MAX_PARTICLE_COUNT)
		amount = MAX_PARTICLE_COUNT - myCount;

	if(amount <= 0) return;

	// Resize particle data arrays
	const int begin = myCount;
	int count = myCount + amount;
	if(count > myReserved)
	{
		const int size = GxMax(myReserved*2, count);
		myData.Resize(size);
		myReserved = size;
	}
	myCount = count;

	// Get pointers to property data
	float* plife = myData[PD_LIFE];
	float* px    = myData[PD_X];
	float* py    = myData[PD_Y];
	float* pz    = myData[PD_Z];

	float* pvx   = myData[PD_VX],   *dvx   = myData[PD_VXD];
	float* pvy   = myData[PD_VY],   *dvy   = myData[PD_VYD];
	float* pvz   = myData[PD_VZ],   *dvz   = myData[PD_VZD];
	float* pr    = myData[PD_R],    *dr    = myData[PD_RD];
	float* pg    = myData[PD_G],    *dg    = myData[PD_GD];
	float* pb    = myData[PD_B],    *db    = myData[PD_BD];
	float* protX = myData[PD_ROTX], *drotX  = myData[PD_ROTXD];
	float* protY = myData[PD_ROTY], *drotY  = myData[PD_ROTYD];
	float* protZ = myData[PD_ROTZ], *drotZ  = myData[PD_ROTZD];
	float* psize = myData[PD_SIZE], *dsize = myData[PD_SIZED];

	// Set position
	for(int i=begin; i<count; ++i)
	{
		px[i] = spawnPos.x;
		py[i] = spawnPos.y;
		pz[i] = spawnPos.z;
	}

	// Set life
	for(int i=begin; i<count; ++i)
		plife[i] = Randf(myType.life.a, myType.life.b);

	// Store direction in the velocity array
	if(pvx && pvy && pvz)
	{
		// Interpolate between random direction and spawn direction
		for(int i=begin; i<count; ++i)
		{
			GxVec3f rd(Randf(-1, 1), Randf(0, 1), Randf(-1, 1));
			if(rd.x != 0 || rd.y != 0)
			{
				float r = 1.f / sqrtf(rd.x*rd.x+rd.y*rd.y+rd.z*rd.z);
				rd.x *= r, rd.y *= r, rd.z *= r;
			}
			else rd = GxVec3f(0, 1, 0);

			pvx[i] = GxLerp(spawnDir.x, rd.x, myType.spawnSpread);
			pvy[i] = GxLerp(spawnDir.y, rd.y, myType.spawnSpread);
			pvz[i] = GxLerp(spawnDir.z, rd.z, myType.spawnSpread);
		}
	}

	// Calculate velocity and velocity delta
	const float a = myType.vel.a, b = myType.vel.b;
	const float c = myType.vel.c, d = myType.vel.d;

	for(int i=begin; i<count; ++i)
	{
		const float value = Randf(a, b);
		const float delta = (Randf(c, d) - value) / plife[i];

		dvx[i] = pvx[i] * delta;
		dvy[i] = pvy[i] * delta;
		dvz[i] = pvz[i] * delta;

		pvx[i] *= value;
		pvy[i] *= value;
		pvz[i] *= value;
	}

	// Initialize miscellaneous properties
	InitProperty(myType.r,    pr,    dr,    plife, begin, count);
	InitProperty(myType.g,    pg,    dg,    plife, begin, count);
	InitProperty(myType.b,    pb,    db,    plife, begin, count);
	InitProperty(myType.rotX, protX, drotX, plife, begin, count);
	InitProperty(myType.rotY, protY, drotY, plife, begin, count);
	InitProperty(myType.rotZ, protZ, drotZ, plife, begin, count);
	InitProperty(myType.size, psize, dsize, plife, begin, count);
}

void GxParticleEmitter::Tick(float dt)
{
	if(myCount == 0) return;

	// Get pointers to property data
	float* plife = myData[PD_LIFE];
	float* px    = myData[PD_X];
	float* py    = myData[PD_Y];
	float* pz    = myData[PD_Z];

	float* pvx   = myData[PD_VX],   *dvx   = myData[PD_VXD];
	float* pvy   = myData[PD_VY],   *dvy   = myData[PD_VYD];
	float* pvz   = myData[PD_VZ],   *dvz   = myData[PD_VZD];
	float* pr    = myData[PD_R],    *dr    = myData[PD_RD];
	float* pg    = myData[PD_G],    *dg    = myData[PD_GD];
	float* pb    = myData[PD_B],    *db    = myData[PD_BD];
	float* protX = myData[PD_ROTX], *drotX  = myData[PD_ROTXD];
	float* protY = myData[PD_ROTY], *drotY  = myData[PD_ROTYD];
	float* protZ = myData[PD_ROTZ], *drotZ  = myData[PD_ROTZD];
	float* psize = myData[PD_SIZE], *dsize = myData[PD_SIZED];

	// Update life, create a list of dead particles
	myDeadParticles.clear();
	for(int i=0; i<myCount; ++i)
	{
		plife[i] -= dt;
		if(plife[i] <= 0.0f)
			myDeadParticles.push_back(i);
	}
	const int deadParticleCount = (int)myDeadParticles.size();
	const int* deadParticles = deadParticleCount ? (&myDeadParticles.front()) : NULL;

	// Replace dead particle data with data from the back of the list
	for(int i=0; i<PD_COUNT; ++i)
	{
		int last = myCount;
		float* data = myData.data[i];
		for(int j=0; j<deadParticleCount; ++j)
			data[deadParticles[j]] = data[--last];
	}
	for(int i=0; i<PD_COUNT; ++i) 
	{
		int last = myCount;
		float* data = myData.data[i];
		for(int j=0; j<deadParticleCount; ++j)
			data[deadParticles[j]] = data[--last];
	}
	myCount -= deadParticleCount;

	// Update position
	if(pvx && pvy)
	{
		for(int i=0; i<myCount; ++i)
		{
			px[i] += dt * pvx[i];
			py[i] += dt * pvy[i];
			pz[i] += dt * pvz[i];
		}

		// Update velocity
		if(dvx && dvy && dvz)
		{
			float forceX = myType.externalForce.x * dt;
			float forceY = myType.externalForce.y * dt;
			float forceZ = myType.externalForce.z * dt;

			for(int i=0; i<myCount; ++i)
			{
				pvx[i] += forceX + dvx[i] * dt;
				pvy[i] += forceY + dvy[i] * dt;
				pvz[i] += forceZ + dvz[i] * dt;
			}
		}
	}
	
	// Update misc properties
	TickProperty(myType.r, pr, dr, dt);
	TickProperty(myType.g, pg, dg, dt);
	TickProperty(myType.b, pb, db, dt);
	TickProperty(myType.rotX, protX, drotX, dt);
	TickProperty(myType.rotY, protY, drotY, dt);
	TickProperty(myType.rotZ, protZ, drotZ, dt);
	TickProperty(myType.size, psize, dsize, dt);
}

void GxParticleEmitter::Render()
{
	const int count = myCount;
	if(count == 0) return;

	// Enable lighting
	glColor3f(1, 1, 1);
	glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_NORMALIZE);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_DIFFUSE);

	// Set blend mode
	glEnable(GL_BLEND);
	switch(myType.blendMode)
	{
		case 0: glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); break;
		case 1: glBlendFunc(GL_SRC_ALPHA, GL_ONE                ); break;
		case 2: glBlendFunc(GL_DST_COLOR, GL_ONE_MINUS_SRC_ALPHA); break;
	};

	// Get pointers to property data
	const float* px    = myData[PD_X];
	const float* py    = myData[PD_Y];
	const float* pz    = myData[PD_Z];
	const float* protX  = myData[PD_ROTX];
	const float* protY  = myData[PD_ROTY];
	const float* protZ  = myData[PD_ROTZ];
	const float* psize = myData[PD_SIZE];
	const float* pr    = myData[PD_R];
	const float* pg    = myData[PD_G];
	const float* pb    = myData[PD_B];

	// Set vertex data
	glDisableClientState(GL_COLOR_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);
	glEnable(GL_NORMALIZE);

	int vcount = 0;
	switch(myType.model)
	{
	case 0:
		glVertexPointer(3, GL_FLOAT, 0, quadVerts);
		glNormalPointer(GL_FLOAT, 0, quadNormals);
		vcount = quadVCount;
		break;
	
	case 1:
		glVertexPointer(3, GL_FLOAT, 0, squareVerts);
		glNormalPointer(GL_FLOAT, 0, squareNormals);
		vcount = squareVCount;
		break;
	};	
	
	glMatrixMode(GL_MODELVIEW_MATRIX);
	glPushMatrix();

	for(int i=0; i<count; ++i)
	{
		glPushMatrix();
	
		glTranslatef(px[i], py[i], pz[i]);
		glScalef(psize[i], psize[i], psize[i]);
		glRotatef(protX[i], 1, 0, 0);
		glRotatef(protY[i], 0, 1, 0);
		glRotatef(protZ[i], 0, 0, 1);
		glColor3f(pr[i], pg[i], pb[i]);

		glDrawArrays(GL_QUADS, 0, vcount);
		glPopMatrix();
	}
	
	glDisable(GL_NORMALIZE);
	glDisableClientState(GL_NORMAL_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);

	glDisable(GL_LIGHTING);

	GxRenderInterface* renderInterface = GxRenderInterface::Get();
	renderInterface->SetBlendMode(GX_BM_ALPHA);
}

}; // namespace particles
}; // namespace guix