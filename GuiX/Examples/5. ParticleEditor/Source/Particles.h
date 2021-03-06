#pragma once

#include <vector>
#include <map>

#include <GuiX/Common.h>
#include <GuiX/String.h>
#include <GuiX/Texture.h>
#include <GuiX/Interfaces.h>

namespace guix {
namespace particles {

class GxParticleEmitter;

// ===================================================================================
// GxParticleData
// ===================================================================================

// GxParticleData, particle data fields
// ParticleLists always have arrays for these data types
enum GxParticleDataField {

	PD_LIFE,
	PD_X,
	PD_Y,
	PD_Z,
	PD_R,    PD_RD,
	PD_G,    PD_GD,
	PD_B,    PD_BD,
	PD_VX,   PD_VXD,
	PD_VY,   PD_VYD,
	PD_VZ,   PD_VZD,
	PD_ROTX, PD_ROTXD,
	PD_ROTY, PD_ROTYD,
	PD_ROTZ, PD_ROTZD,
	PD_SIZE, PD_SIZED,
	
	PD_COUNT
};

// ParticleData, used by ParticleLists
// Contains all data for updating particles of a particular type
struct GxParticleData
{
	GxParticleData(); 
	~GxParticleData() {Clear();}

	void Clear();
	void Resize(int size);

	inline       float* operator [] (GxParticleDataField i)       {return data[i];}
	inline const float* operator [] (GxParticleDataField i) const {return data[i];}

	float* data[PD_COUNT];
};

// ===================================================================================
// GxParticleProperty
// ===================================================================================

// GxPropertyValue, describes how the value of a property is calculated for a particle
enum GxPropertyValue {

	PV_ZERO,          // The value is zero
	PV_CONSTANT,      // The valua is a constant
	PV_RANDOM,        // The value is a random number
};

// GxPropertyDelta, describes how the delta value of a property is calculated for a particle
enum GxPropertyDelta {

	PD_ZERO,             // The delta is zero
	PD_CONSTANT,         // The delta is a constant
	PD_RANDOM,           // The delta is a random number
	PD_LERP_TO_CONSTANT, // The delta is based on interpolation to a constant
	PD_LERP_TO_RANDOM,   // The delta is based on interpolation to a random value
};

// GxParticleProperty, contains information for calculating the initial value and delta of a property for a particle
struct GxParticleProperty
{
	GxParticleProperty()                       : a(0),     b(0),     c(0),   d(0),   value(PV_ZERO),     delta(PD_ZERO) {}
	GxParticleProperty(float val)              : a(val),   b(val),   c(0),   d(0),   value(PV_CONSTANT), delta(PD_ZERO) {}
	GxParticleProperty(float begin, float end) : a(begin), b(begin), c(end), d(end), value(PV_CONSTANT), delta(PD_LERP_TO_CONSTANT) {}

	GxParticleProperty(float _a, float _b, float _c, float _d)
		: a(_a), b(_b), c(_c), d(_d), value(PV_RANDOM), delta(PD_LERP_TO_RANDOM) {}

	void OptimizeFlags();

	float a, b, c, d;
	GxPropertyValue value;
	GxPropertyDelta delta;
};

// ===================================================================================
// GxParticleType
// ===================================================================================

// GxParticleType, contains all information that defines a type of particle
struct GxParticleType
{
	GxParticleType();

	GxParticleProperty life;
	GxParticleProperty r,g,b;
	GxParticleProperty vel;
	GxParticleProperty rotX, rotY, rotZ;
	GxParticleProperty size;
	GxVec3f externalForce;
	float spawnSpread;
	int blendMode;
	int model;
};

// ===================================================================================
// GxParticleEmitter
// ===================================================================================

class GxParticleEmitter
{
public:
	enum {MAX_PARTICLE_COUNT = 0x10000};

	GxParticleEmitter();
	~GxParticleEmitter();

	void SetType(const GxParticleType& type);

	void InitProperty(const GxParticleProperty& range, float* values, float* deltas, float* life, int begin, int count);
	void TickProperty(const GxParticleProperty& range, float* values, float* deltas, float dt);

	void Spawn(const GxVec3f& pos, const GxVec3f& dir, int amount);
	void Tick(float dt);
	void Render();

	int GetParticleCount() {return myCount;}
	GxParticleType& GetType() {return myType;}

private:
	GxParticleType myType;
	GxParticleData myData;
	int myCount, myReserved;
	std::vector<int> myDeadParticles;
};

}; // namespace particles
}; // namespace guix