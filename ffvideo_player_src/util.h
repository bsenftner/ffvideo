#pragma once

#ifndef _UTIL_H_
#define _UTIL_H_



#define FF_ABS(x) (x < 0 ? -x : x)
inline int32_t ff_abs(const int32_t X) { return FF_ABS(X); };
inline float ff_fabs(const float X) { return FF_ABS(X); };
inline int32_t ff_round(const float rX) { return ((int)(rX < 0 ? (rX - 0.5f) : (rX + 0.5f))); };
inline float ff_ceil(const float rX) { return (float)ceil((double)rX); };

////////////////////////////////////////////////////////////////////////////////
class FF_Coord2D {
public:
	FF_Coord2D(int32_t a, int32_t b) { x = a; y = b; };
	FF_Coord2D() { x = 0; y = 0; }
	FF_Coord2D(const FF_Coord2D& src) { x = src.x; y = src.y; }
	~FF_Coord2D() {};

	void Set(int32_t a, int32_t b) { x = a; y = b; };
	void Scale(int32_t s) { x *= s; y *= s; };
	void Scale(FF_Coord2D s) { x *= s.x; y *= s.y; };

	int32_t x, y;
};

////////////////////////////////////////////////////////////////////////////////
class FF_Vector2D {
public:
	FF_Vector2D(float a, float b) { x = a; y = b; };
	FF_Vector2D() { x = 0.0; y = 0.0; }
	FF_Vector2D(const FF_Vector2D& src) { x = src.x; y = src.y; }
	~FF_Vector2D() {};

	void Set(float a, float b) { x = a; y = b; };
	void Set(const FF_Vector2D& src) { x = src.x; y = src.y; };
	void Scale(float s) { x *= s; y *= s; };
	void Scale(FF_Vector2D s) { x *= s.x; y *= s.y; };

	float x, y;
};

////////////////////////////////////////////////////////////////////////////////
class FF_Vector3D {
public:
	FF_Vector3D(float a, float b, float c) { x = a; y = b; z = c; };
	FF_Vector3D() { x = 0.0; y = 0.0; z = 0.0; }
	FF_Vector3D(const FF_Vector3D& src) { x = src.x; y = src.y; z = src.z; }
	~FF_Vector3D() {};

	void Set(float a, float b, float c) { x = a; y = b; z = c; };
	void Scale(float s) { x *= s; y *= s; z *= s; };
	void Scale(FF_Vector3D s) { x *= s.x; y *= s.y; z *= s.z; };

	float x, y, z;
};



#endif // _UTIL_H_

