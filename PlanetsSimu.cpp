// PlanetsSimu.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <vector>
#include <map>
#include <random>

#define PI 3.141592653589
#define E  2.718281828459 
#define G  6674
#define EPS0 0.000001
#define DEFAULTMASS 1
#define TOTAL_METHODS 8
#define METHOD_EULER_EX 1
#define METHOD_EULER_IM 2
#define METHOD_RK2_EX 3
#define METHOD_RK2_IM 4
#define METHOD_RK2_HF 5
#define METHOD_RK4_EX 6
#define METHOD_RK4_IM 7
#define METHOD_RK4_HF 8

using namespace std;

class Point3d {
public:
	Point3d() :x{ 0 }, y{ 0 }, z{ 0 } {};
	Point3d(double xx, double yy = 0, double zz = 0) :
		x{ xx }, y{ yy }, z{ zz } {};

	double x, y, z;
	//this Norm does not have a minimum
	double Norm() const { return sqrt(x*x + y*y + z*z); }
	bool IsZero() const { return (this->Norm() < EPS0) ? true : false; }

	double dot(Point3d p) { return x*p.x + y*p.y + z*p.z; }
	void cross(Point3d);

	Point3d& operator+=(Point3d p);
	Point3d& operator-=(Point3d p);
};

void Point3d::cross(Point3d p) {
	double xx, yy, zz;
	xx = y*p.z - z*p.y;
	yy = z*p.x - x*p.z;
	zz = x*p.y - y*p.x;
	x = xx; y = yy; z = zz;
}
Point3d operator+(const Point3d& p1, const Point3d& p2) { return Point3d{ p1.x + p2.x,p1.y + p2.y,p1.z + p2.z }; }
Point3d operator-(const Point3d& p1, const Point3d& p2) { return Point3d{ p1.x - p2.x,p1.y - p2.y,p1.z - p2.z }; }

Point3d operator*(double k, Point3d p) { return Point3d{ k*p.x,k*p.y,k*p.z }; }
Point3d& Point3d::operator+=(Point3d p) { x += p.x; y += p.y; z += p.z; return *this; }
Point3d& Point3d::operator-=(Point3d p) { x -= p.x; y -= p.y; z -= p.z; return *this; }
bool operator==(const Point3d& p1, const Point3d& p2) { Point3d p = p1 - p2; return p.IsZero(); }
//this Norm has a minimum of EPS0
double Norm(const Point3d& p) { return (p.IsZero()) ? EPS0 : sqrt(p.x*p.x + p.y*p.y + p.z*p.z); }

class MassPoint {
public:
	MassPoint() :ID{ -1 }, pos{ Point3d{} }, vel{ Point3d{} }, mass{ DEFAULTMASS } {};
	//the ID & position is required
	MassPoint(int id, Point3d r, Point3d v = Point3d(), double m = DEFAULTMASS) :
		pos{ r }, vel{ v }, mass{ m }, ID{ id } {};
	MassPoint(int id, const MassPoint& m) :
		pos{ m.pos }, vel{ m.vel }, mass{ m.mass }, ID{ id } {};

	Point3d pos;
	Point3d vel;
	double mass;
	int ID;
};

//a world contains the planets and sunmass
//to add planets, please use AddPlanet() or AddFromKeyboard()
class World {
public:
	World(double m = DEFAULTMASS) :SunMass{ m } {};
	World(const World& w) :SunMass{ w.SunMass }, Planets{ w.Planets } {};

	map<int, MassPoint> Planets;
	double SunMass;

	void AddPlanet(const MassPoint&);
	void DeletePlanet(const int&);
	void AddFromKeyboard();
	void Create();

	World& operator=(const World& w);

private:
	int StepCount{ 0 };
};
void World::AddPlanet(const MassPoint& p) { Planets[p.ID] = p; }
void World::DeletePlanet(const int& id) { Planets.erase(id); }
World& World::operator=(const World& w) {
	Planets = w.Planets;
	SunMass = w.SunMass;
	return *this;
}

ostream& operator << (ostream& os, const World& w) {
	os << "SunMass=" << w.SunMass << ", Num=" << w.Planets.size() << endl;
	for (auto i = w.Planets.begin(); i != w.Planets.end(); i++) {
		os << "ID: " << i->first
			<< ", pos=(" << i->second.pos.x << "," << i->second.pos.y << "," << i->second.pos.z << ")"
			<< ", vel=(" << i->second.vel.x << "," << i->second.vel.y << "," << i->second.vel.z << ")" << endl;
	}
	os << "----------";
	return os;
}

//The Solver offers different methods to solve the equation, tracing planets info at the same time
class Solver {
public:
	//m=method, w=initial World
	Solver(int m, const World& w) :method{ m }, IniWorld{ w }, PreWorld{ w } {};

	//solve a single step(only applies on single-step mmethods)
	void Step();
	//solve for many steps
	void Solve(int step);
	void ChangeMethod(int m);
	void ChangeRelErr(double e) { eRel = e; }
	void ChangeAbsErr(double e) { eAbs = e; }
	void ChangeMaxRadius(double r) { rMax = r; }
	//Only reset the world but not the err
	void ReSet();

	World GetWorld()const { return PreWorld; }
	//Print Solver parameters to os
	void PrintDetails(ostream& os);

private:
	//see the #define
	int method;
	//IniWorld stays constant, while Preworld processes the simulation 
	World IniWorld, PreWorld;
	//the Border
	double rMax = 4000;
	//step length
	double h = 0.00001;
	//to avoid divergence, set hMax
	double hMax = 0.2;
	//expected precision, relative and absolute
	double eRel = 0.00000001;
	double eAbs = 0.0001;


	map<int, Point3d> VelDiff(const World&);
	World& UpdateWorld(World&, map<int, Point3d>, double);
	double DeltaPos(const World&, const World&);
	void DetectBorder();

	void StepEulerEx();
	//void StepEulerIm();
	//void StepRK2Ex();
	//void StepRK2Im();
	//void StepRK2Hf();
};

void Solver::Step() {
	switch (method) {
	case METHOD_EULER_EX:
		StepEulerEx();
		break;
		/*case METHOD_EULER_IM:
		StepEulerIm();
		break;
		case METHOD_RK2_EX:
		StepRK2Ex();
		break;
		case METHOD_RK2_IM:
		StepRK2Im();
		break;
		case METHOD_RK2_HF:
		StepRK2Hf();
		break;
		*/
		//use Euler method by default
	default:
		StepEulerEx();
		break;
	}
}

void Solver::Solve(int step) {
	for (int i = 0; i < step; i++) { Step(); }
}

void Solver::ChangeMethod(int m) {
	if (m > 0 && m <= TOTAL_METHODS) method = m;
	else throw runtime_error("Method: Unknown Method");
}

void Solver::ReSet() { PreWorld = IniWorld; }

void Solver::PrintDetails(ostream & os) {
	os << "step length(h)=" << h << ", err=" << eAbs + eRel*rMax << endl;
}

map<int, Point3d> Solver::VelDiff(const World& w) {
	map<int, Point3d> f;
	for (auto i = w.Planets.begin(); i != w.Planets.end(); i++) {
		//the sun gravity
		f[i->first] +=
			-(G*i->second.mass*w.SunMass)
			/ pow(Norm(i->second.pos), 3)*(i->second.pos);
		//other planets' gravity
		for (auto j = w.Planets.begin(); j != w.Planets.end(); j++) {
			if (i->first != j->first) {
				f[i->first] +=
					-(G*i->second.mass*j->second.mass)
					/ pow(Norm(i->second.pos - j->second.pos), 3)*(i->second.pos - j->second.pos);
			}
		}
	}
	return f;
}

World& Solver::UpdateWorld(World& ww, map<int, Point3d> ff, double hh) {
	for (auto i = ww.Planets.begin(); i != ww.Planets.end(); i++) {
		i->second.pos += hh*i->second.vel;
		i->second.vel += hh*ff[i->first];
	}
	return ww;
}

double Solver::DeltaPos(const World& w1, const World& w2) {
	if (w1.Planets.size() != w2.Planets.size()) throw runtime_error("Solver: planet number does not match");
	double d{ 0 };
	for (auto i = w2.Planets.begin(); i != w2.Planets.end(); i++) {
		//check 
		w2.Planets.at(i->first).ID;
		//diff
		d += pow(Norm(w1.Planets.at(i->first).pos - w2.Planets.at(i->first).pos), 2);
	}
	return sqrt(d);
}
//only applies on PreWorld
void Solver::DetectBorder() {
	for (auto i = PreWorld.Planets.begin(); i != PreWorld.Planets.end(); ) {
		if (i->second.pos.Norm() > rMax) {
			cout << "Planet " << i->first << " is dead!" << endl;
			PreWorld.Planets.erase(i++);
		}
		else i++;
	}
	cout << "Border check finished!" << endl;
}

void Solver::StepEulerEx() {
	//Calculate f, from PreWorld (Explicit)
	//VelDiff(w1);
	//Estimate err
	//this coefficient comes from err esimation
	const int ErrCoe = 2;

	//! not rMax, but rMax();
	double e = eRel*rMax + eAbs;
	double d{ 0 };
	World w1{ PreWorld };
	World w2{ PreWorld };
	UpdateWorld(w1, VelDiff(w1), h);
	UpdateWorld(w2, VelDiff(w2), h / 2);
	UpdateWorld(w2, VelDiff(w2), h / 2);
	d = ErrCoe*DeltaPos(w1, w2);

	//Change step length
	if (d<e / 3 || d>e) {
		while (d < e && h<hMax) {
			h *= 2;
			w1 = PreWorld;
			w2 = PreWorld;
			UpdateWorld(w1, VelDiff(w1), h);
			UpdateWorld(w2, VelDiff(w2), h / 2);
			UpdateWorld(w2, VelDiff(w2), h / 2);
			d = ErrCoe*DeltaPos(w1, w2);

			cout << "step length doubled, now h=" << h << endl;
		}
		//！距离过近会造成数值精确度严重下降，能量不守恒
		while (d >= e) {
			h /= 2;
			w1 = PreWorld;
			w2 = PreWorld;
			UpdateWorld(w1, VelDiff(w1), h);
			UpdateWorld(w2, VelDiff(w2), h / 2);
			UpdateWorld(w2, VelDiff(w2), h / 2);
			d = ErrCoe*DeltaPos(w1, w2);
			cout << "step length halfed, now h=" << h << endl;
		}
	}

	//Update PreWorld
	PreWorld = w2;
	DetectBorder();
}

void World::AddFromKeyboard() {
	cout << "okay, now please set the planets" << endl
		<< "Format: x y z vx vy vz mass" << endl;
	double x, y, z, vx, vy, vz, mass;
	int id = 0;
	while (cin >> x >> y >> z >> vx >> vy >> vz >> mass) {
		id++;
		MassPoint p(id, Point3d{ x,y,z }, Point3d{ vx,vy,vz }, mass);
		AddPlanet(p);
	}
}

// an easy way to create planets
// random planets (for test)
void World::Create() {
	double x, y, z, vx, vy, vz, mass;
	int id = 0;
	random_device rd;  // 将用于为随机数引擎获得种子
	std::mt19937 gen(rd()); // 以播种标准 mersenne_twister_engine
	// max r=2000, can be modified
	std::uniform_int_distribution<> dis(10, 2000);
	
	while (id<10) {
		id++;
		mass = 1;
		// 用 dis 变换 gen 所生成的随机 unsigned int 到 [10, 2000] 中的 int
		x = dis(gen); y = dis(gen); z = dis(gen);
		vx = double(dis(gen)) / 500; vy = double(dis(gen)) / 500; vz = double(dis(gen)) / 500;
		MassPoint p(id, Point3d{ x,y,z }, Point3d{ vx,vy,vz }, mass);
		AddPlanet(p);
	}
}

int main()
{
	//create world
	//cout << "Welcome! Please enter the mass of the sun (typically 10000): ";
	double m_SunMass{ 100 };
	World m_World{ m_SunMass };
	m_World.Create();

	//create solver
	Solver m_Solver{ METHOD_EULER_EX,m_World };
	m_Solver.ChangeAbsErr(0.2);
	m_Solver.ChangeRelErr(0);

	ofstream ofs{ "output.txt" };
	ofs << "Result:" << endl;
	for (int i = 0; i < 2000; i++) {
		m_Solver.Step();
		m_Solver.PrintDetails(ofs);
		ofs << m_Solver.GetWorld() << endl;
	}

	return 0;
}
