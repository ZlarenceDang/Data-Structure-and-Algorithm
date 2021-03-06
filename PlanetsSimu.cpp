// PlanetsSimu.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <string>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <vector>
#include <map>
#include <random>
#include <Windows.h>
#include <iomanip>

#define PI 3.141592653589
#define E  2.718281828459 
#define G  0.00011840
#define SUNMASS 332968
#define EPS0 0.00000000001
#define DEFAULTMASS 1
#define TOTAL_METHODS 8
#define METHOD_EULER_EX 1
#define METHOD_EULER_IM 2
#define METHOD_FROG 3
#define METHOD_VERLET 4
#define METHOD_RK4_EX 5
#define METHOD_RK4_IM 6
#define BOUND_NONE 1
#define BOUND_KILL 2
#define BOUND_BOX 3

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
	Point3d& operator=(Point3d p);
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
Point3d & Point3d::operator=(Point3d p) { x = p.x; y = p.y; z = p.z; return *this; }
bool operator==(const Point3d& p1, const Point3d& p2) { Point3d p = p1 - p2; return p.IsZero(); }
//this Norm has a minimum of EPS0
double Norm(const Point3d& p) { return (p.IsZero()) ? EPS0 : sqrt(p.x*p.x + p.y*p.y + p.z*p.z); }

typedef map<int, Point3d> dVel;

typedef struct {
	dVel dpos;
	dVel dvel;
} dState;

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
	bool InBound{ true };
	
	MassPoint& operator = (const MassPoint&);
};
MassPoint& MassPoint:: operator = (const MassPoint& m) {
	pos = m.pos;	vel = m.vel;	mass = m.mass;	ID = m.ID;	InBound = m.InBound;
	return *this;
}
//a world contains the planets and sunmass
//to add planets, please use AddPlanet() or AddFromKeyboard()
class World {
public:
	World(double m = SUNMASS) :SunMass{ m } {};
	World(const World& w) :SunMass{ w.SunMass }, Planets{ w.Planets } {};

	map<int, MassPoint> Planets;
	double SunMass;

	double rMax();

	void AddPlanet(const MassPoint&);
	void DeletePlanet(const int&);
	void AddFromKeyboard();
	void RandomCreate(int);
	void AddFromFile(ifstream&);
	double CalcEnergy();
	dVel CalcdPos();
	

	World& operator=(const World& w);

private:
	int StepCount{ 0 };
	
};
double World::rMax()
{
	double m{ 0 };
	for (auto i = Planets.begin(); i != Planets.end(); i++) { m = (Norm(i->second.pos) > m) ? Norm(i->second.pos) : m; }
	return m;
}
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



dVel operator*(double k, const dVel& V){
	dVel VV = V;
	for (auto i = V.begin(); i != V.end(); i++) { VV.at(i->first) = k*i->second; }
	return VV;
}
dVel operator+(const dVel& V1, const dVel& V2) {
	dVel VV = V1;
	for (auto i = V1.begin(); i != V1.end(); i++) { VV.at(i->first) += V2.at(i->first); }
	return VV;
}

//The Solver offers different methods to solve the equation, tracing planets info at the same time
class Solver {
public:
	//m=method, w=initial World
	Solver(int m, int b, const World& w) :method{ m }, bound{ b }, IniWorld{ w }, PreWorld{ w } {};

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
	void SetIniEnergy(double d) { IniEnergy = d; }

	World GetWorld()const { return PreWorld; }
	//Print Solver parameters to os
	void PrintDetails(ostream&);
	void PrintDistribution(ostream& , string);
	void PrintAnalysis(ostream&);

private:
	//see the #define
	int method, bound;
	//IniWorld stays constant, while Preworld processes the simulation 
	World IniWorld, PreWorld;
	//the Border
	double rMax = 10000;
	//step length
	double h = 0.00001;
	//to avoid divergence, set hMax
	double hMax = 0.2;
	//expected precision, relative and absolute
	double eRel = 0.00000001;
	double eAbs = 0.0001;
	//estimated err
	double EstimatedErr;
	//single step time
	double StepTime;
	//single time per unit simu time
	double UnitTime;
	//total system time
	double SysTimeTot{ 0 };
	//total simu time
	double SimuTimeTot{ 0 };
	//initial energy
	double IniEnergy{ 0 };

	dVel VelDiff(const World&);
	dState VelDiffRK4(const World&, double hh);

	//world, delta_pos, delta_vel, delta_t
	World& UpdateWorld(World&, dVel, dVel, double);
	double DeltaPos(const World&, const World&);

	//input: BOUND_X 
	//@switch bound behaviour
	//@only applies on PreWorld 
	void DetectBorder(int);

	void StepEulerEx();
	void StepEulerIm();
	void StepFrog();
	void StepVerlet();
	void StepRK4Ex();
	void StepRK4Im();

	World& StepEulerImStep(World&, double);
	World& StepFrogStep(World&, double);
	World& StepVerletStep(World&, double);
	World& StepRK4ExStep(World&, double);
	World& StepRK4ImStep(World&, double);
};

//record time
void Solver::Step() {
	LARGE_INTEGER t1, t2, tc;
	QueryPerformanceFrequency(&tc);
	QueryPerformanceCounter(&t1);
	
	switch (method) {
	case METHOD_EULER_EX:
		StepEulerEx();
		break;
	case METHOD_EULER_IM:
		StepEulerIm();
		break;
	case METHOD_FROG:
		StepFrog();
		break;
	case METHOD_VERLET:
		StepVerlet();
		break;
	case METHOD_RK4_EX:
		StepRK4Ex();
		break;
	case METHOD_RK4_IM:
		StepRK4Im();
		break;
	//use Euler method by default
	default:
		StepEulerIm();
		break;
	}
	QueryPerformanceCounter(&t2);
	StepTime = (t2.QuadPart - t1.QuadPart)*1.0 / tc.QuadPart;
	UnitTime = StepTime / h;
	SysTimeTot += StepTime;
	SimuTimeTot += h;
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
	os.setf(ios::fixed);
	os.precision(12);
	os << "step length(h)=" << h << ", err=" << eAbs + eRel*PreWorld.rMax()
		<< ", estimated err=" << EstimatedErr
		<< ", step time=" << StepTime
		<< ", step time ratio=" << UnitTime
		<< ", totaltime=" << SysTimeTot
		<< ", average ratio=" << SysTimeTot / SimuTimeTot
		<< endl;
}

int Nearest10Up(double d) { return 10 * ceil(int(ceil(d)) / 10); }
int Nearest10Down(double d) { return 10 * floor(int(floor(d)) / 10); }

template<typename T>
ostream& operator<<(ostream& os, vector<T> v) {
	for (auto i = v.begin(); i != v.end(); i++) { os << *i << endl; }
	return os;
}
void Solver::PrintDistribution(ostream & os, string s)
{	
	int n = 50;
	os.setf(ios::fixed);
	os.precision(12);
	vector<double> vx, vy, vz, v;
	for (auto i = PreWorld.Planets.begin(); i != PreWorld.Planets.end(); i++) {
		vx.push_back(i->second.vel.x);
		vy.push_back(i->second.vel.y);
		vz.push_back(i->second.vel.z);
		v.push_back(sqrt(i->second.vel.x*i->second.vel.x+ i->second.vel.y*i->second.vel.x+ i->second.vel.z*i->second.vel.z));
		sort(vx.begin(), vx.end());
		sort(vy.begin(), vy.end());
		sort(vz.begin(), vz.end());
		sort(v.begin(), v.end());
	}
	/*double vxlt{ floor(*vx.begin()) };
	double vxrt{ ceil(*(vx.end()-1)) };
	double dvx{ (vxrt - vxlt)*1.0 / n };
	vector<int> vxdis(n, 0);
	for (int i = 0; i < vx.size(); i++) {
		vxdis.at(floor((vx.at(i) - vxlt)*1.0 / dvx))++;
	}
	for (int i = 0; i < n; i++) {
		os << "vx\t" << vxlt + (i + 0.5)*dvx << "\tn\t" << vxdis.at(i) << endl;
	}*/
	ofstream fDis("VxDis_" + s + ".txt",ios::out);
	fDis << vx;
	fDis.close();
}

void Solver::PrintAnalysis(ostream &os)
{
	os.setf(ios::fixed);
	os.precision(12);
	os << "step length(h)\t" << h
		//<< "\terr\t" << eAbs + eRel*PreWorld.rMax()
		<< "\testimated err\t" << EstimatedErr
		<< "\tstep time\t" << StepTime
		<< "\tstep time ratio\t" << UnitTime
		<< "\ttotaltime\t" << SysTimeTot
		<< "\taverage ratio\t" << SysTimeTot / SimuTimeTot
		<< "\tsimutime\t" << SimuTimeTot
		<< "\ttotal energy\t" << PreWorld.CalcEnergy()
		<< "\tenergy err\t" << abs((PreWorld.CalcEnergy() - IniEnergy) / IniEnergy)
		<< "\tdelta_d0\t" << PreWorld.Planets.begin()->second.pos.Norm() - 1.0
		<< endl;
}

dVel Solver::VelDiff(const World& w) {
	dVel f;
	for (auto i = w.Planets.begin(); i != w.Planets.end(); i++) {
		//the sun gravity
		f[i->first] +=
			-(G*w.SunMass)
			/ pow(Norm(i->second.pos), 3)*(i->second.pos);
		//other planets' gravity
		for (auto j = w.Planets.begin(); j != w.Planets.end(); j++) {
			if (i->first != j->first) {
				f[i->first] +=
					-(G*j->second.mass)
					/ pow(Norm(i->second.pos - j->second.pos), 3)*(i->second.pos - j->second.pos);
			}
		}
	}
	return f;
}

dState Solver::VelDiffRK4(const World &ww, double hh)
{
	World w1{ ww };
	dVel v1, v2, v3, v4, dv;
	dVel p1, p2, p3, p4, dp;

	p1 = w1.CalcdPos();
	v1 = VelDiff(w1);
	UpdateWorld(w1, p1, v1, hh / 2);

	p2 = w1.CalcdPos();
	v2 = VelDiff(w1);
	w1 = ww;
	UpdateWorld(w1, p2, v2, hh / 2);

	p3 = w1.CalcdPos();
	v3 = VelDiff(w1);
	w1 = ww;
	UpdateWorld(w1, p3, v3, hh);

	p4 = w1.CalcdPos();
	v4 = VelDiff(w1);

	dp = 1.0 / 6 * (p1 + 2 * p2 + 2 * p3 + p4);
	dv = 1.0 / 6 * (v1 + 2 * v2 + 2 * v3 + v4);

	return dState{ dp, dv };
}

World& Solver::UpdateWorld(World& ww, dVel dp, dVel dv, double hh) {
	for (auto i = ww.Planets.begin(); i != ww.Planets.end(); i++) {
		dp.try_emplace(i->first, Point3d{});
		dv.try_emplace(i->first, Point3d{});
		i->second.pos += hh*dp.at(i->first);
		i->second.vel += hh*dv.at(i->first);
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

void Solver::DetectBorder(int n) {
	for (auto i = PreWorld.Planets.begin(); i != PreWorld.Planets.end(); ) {
		switch (n) {
		case BOUND_NONE:
			i++;
			break;
		case BOUND_KILL:
			if (i->second.pos.Norm() > rMax) {
				cout << "Planet " << i->first << " is dead!" << endl;
				PreWorld.Planets.erase(i++);
			}
			else i++;
			break;
		case BOUND_BOX:
		{
			if (i->second.pos.Norm() > rMax) {
				if (i->second.InBound) {
					i->second.InBound = false;
					cout << "Planet " << i->first << " hits the border!" << endl;
					cout << "v=" << i->second.vel.Norm() << " ,r=" << i->second.pos.Norm();
					Point3d v{ i->second.vel };
					//the unit vector pointing out of the surface
					Point3d p{ 1.0 / i->second.pos.Norm() * i->second.pos };
					cout << "v.n=" << v.dot(p) << endl;
					if (v.dot(p) > 0)	i->second.vel -= 2 * v.dot(p)*p;
				}
				i++;
			}
			else { i++; i->second.InBound = true; }
			break;
		}
		default:
			if (i->second.pos.Norm() > rMax) {
				cout << "Planet " << i->first << " is dead!" << endl;
				PreWorld.Planets.erase(i++);
			}
			else i++;
			break;
		}
	}
	//cout << "Border check finished!" << endl;
}

void Solver::StepEulerEx() {
	//Calculate f, from PreWorld (Explicit)
	//VelDiff(w1);
	//Estimate err
	//this coefficient comes from err esimation
	const int ErrCoe = 2;

	double e = eRel*PreWorld.rMax() + eAbs;
	double d{ 0 };
	World w1{ PreWorld };
	World w2{ PreWorld };
	UpdateWorld(w1, w1.CalcdPos(), VelDiff(w1), h);
	UpdateWorld(w2, w2.CalcdPos(), VelDiff(w2), h / 2);
	UpdateWorld(w2, w2.CalcdPos(), VelDiff(w2), h / 2);
	d = ErrCoe*DeltaPos(w1, w2);

	//Change step length and Estimated err
	if (d<e / 10 || d>= e) {
		while (d < e / 10 && h<hMax) {
			h *= 2;
			w1 = PreWorld;
			w2 = PreWorld;
			UpdateWorld(w1, w1.CalcdPos(), VelDiff(w1), h);
			UpdateWorld(w2, w2.CalcdPos(), VelDiff(w2), h / 2);
			UpdateWorld(w2, w2.CalcdPos(), VelDiff(w2), h / 2);
			d = ErrCoe*DeltaPos(w1, w2);

			cout << "step length doubled, now h=" << h << endl;
		}
		//！距离过近会造成数值精确度严重下降，能量不守恒
		while (d >= e) {
			h /= 2;
			w1 = PreWorld;
			w2 = PreWorld;
			UpdateWorld(w1, w1.CalcdPos(), VelDiff(w1), h);
			UpdateWorld(w2, w2.CalcdPos(), VelDiff(w2), h / 2);
			UpdateWorld(w2, w2.CalcdPos(), VelDiff(w2), h / 2);
			d = ErrCoe*DeltaPos(w1, w2);
			cout << "step length halfed, now h=" << h << endl;
		}
	}
	EstimatedErr = d;

	//Update PreWorld
	PreWorld = w2;
	DetectBorder(bound);
}

World& Solver::StepEulerImStep(World& ww, double hh) {
	const int ErrCoe = 1;
	double e = eRel*ww.rMax() + eAbs;
	double dd{ 0 };
	World w1{ ww };
	World w2{ ww };
	

	//iteration method
	UpdateWorld(w1, w1.CalcdPos(), VelDiff(w1), h);
	UpdateWorld(w2, w1.CalcdPos(), VelDiff(w1), hh);
	dd = ErrCoe*DeltaPos(w1, w2);
	//iterate
	while (dd >= e) {
	//for (int i=0;i<5;i++){
		w1 = w2; w2 = PreWorld;
		UpdateWorld(w2, w1.CalcdPos(), VelDiff(w1), hh);
		dd = ErrCoe*DeltaPos(w1, w2);
	}
	ww = w2;
	return ww;
}

World & Solver::StepFrogStep(World &ww, double hh){
	World w1{ PreWorld };
	UpdateWorld(w1, dVel{}, VelDiff(w1), hh);
	UpdateWorld(w1, w1.CalcdPos(), dVel{}, hh);
	ww = w1;	return ww;
}

World & Solver::StepVerletStep(World &ww, double hh)
{
	World w1{ PreWorld };
	dVel dv1 = VelDiff(w1);
	UpdateWorld(w1, w1.CalcdPos() + 1.0 / 2 * hh*dv1, dVel{}, hh);
	UpdateWorld(w1, dVel{}, 1.0 / 2 * (dv1 + VelDiff(w1)), hh);
	ww = w1;	return ww;
}

World & Solver::StepRK4ExStep(World & ww, double hh)
{
	dState dS = VelDiffRK4(ww, hh);
	World w2{ PreWorld };
	UpdateWorld(w2, dS.dpos, dS.dvel, hh);
	ww = w2;
	return ww;
}

World & Solver::StepRK4ImStep(World &ww, double hh)
{
	const int ErrCoe = 1;
	double e = eRel*ww.rMax() + eAbs;
	double dd{ 0 };
	World w1{ ww };
	World w2{ ww };
	dState dS;


	//iteration method
	dS = VelDiffRK4(w1, hh);
	UpdateWorld(w1, dS.dpos, dS.dvel, hh);
	dS = VelDiffRK4(w1, hh);
	UpdateWorld(w2, dS.dpos, dS.dvel, hh);
	dd = ErrCoe*DeltaPos(w1, w2);
	//iterate
	while (dd >= e) {
		//for (int i=0;i<5;i++){
		w1 = w2; w2 = PreWorld;
		dS = VelDiffRK4(w1, hh);
		UpdateWorld(w2, dS.dpos, dS.dvel, hh);
		dd = ErrCoe*DeltaPos(w1, w2);
	}
	ww = w2;
	return ww;
}

void Solver::StepEulerIm() {
	//Estimate err
	//this coefficient comes from err esimation
	const int ErrCoe = 2;

	double e = eRel*PreWorld.rMax() + eAbs;
	double d{ 0 };
	World w1{ PreWorld };
	World w2{ PreWorld };
	
	//Implicit solution
	StepEulerImStep(w1, h);
	StepEulerImStep(w2, h / 2);
	StepEulerImStep(w2, h / 2);
	d = ErrCoe*DeltaPos(w1, w2);

	//Change step length and Estimated err
	if (d<e / 10 || d>e) {
		while (d < e/10 && h<hMax) {
			h *= 2;
			w1 = PreWorld;
			w2 = PreWorld;
			StepEulerImStep(w1, h);
			StepEulerImStep(StepEulerImStep(w2, h / 2), h / 2);
			d = ErrCoe*DeltaPos(w1, w2);

			cout << "step length doubled, now h=" << h << endl;
		}
		//！距离过近会造成数值精确度严重下降，能量不守恒
		while (d >= e) {
			h /= 2;
			w1 = PreWorld;
			w2 = PreWorld;
			StepEulerImStep(w1, h);
			StepEulerImStep(StepEulerImStep(w2, h / 2), h / 2);
			d = ErrCoe*DeltaPos(w1, w2);

			cout << "step length halfed, now h=" << h << endl;
		}
	}
	EstimatedErr = d;

	//Update PreWorld
	PreWorld = w2;
	DetectBorder(bound);
}

void Solver::StepFrog()
{
	//Estimate err
	//this coefficient comes from err esimation
	const double ErrCoe = 4.0 / 3;

	double e = eRel*PreWorld.rMax() + eAbs;
	double d{ 0 };
	World w1{ PreWorld };
	World w2{ PreWorld };
	StepFrogStep(w1, h);
	StepFrogStep(w2, h / 2);
	StepFrogStep(w2, h / 2);
	d = ErrCoe*DeltaPos(w1, w2);

	//Change step length and Estimated err
	if (d<e / 10 || d >= e) {
		while (d < e / 10 && h<hMax) {
			h *= 2;
			w1 = PreWorld;
			w2 = PreWorld;
			StepFrogStep(w1, h);
			StepFrogStep(w2, h / 2);
			StepFrogStep(w2, h / 2);
			d = ErrCoe*DeltaPos(w1, w2);

			cout << "step length doubled, now h=" << h << endl;
		}
		//！距离过近会造成数值精确度严重下降，能量不守恒
		while (d >= e) {
			h /= 2;
			w1 = PreWorld;
			w2 = PreWorld;
			StepFrogStep(w1, h);
			StepFrogStep(w2, h / 2);
			StepFrogStep(w2, h / 2);
			d = ErrCoe*DeltaPos(w1, w2);
			cout << "step length halfed, now h=" << h << endl;
		}
	}
	EstimatedErr = d;

	//Update PreWorld
	PreWorld = w2;
	DetectBorder(bound);
}

void Solver::StepVerlet()
{
	//Estimate err
	//this coefficient comes from err esimation
	const double ErrCoe = 8.0 / 7;

	double e = eRel*PreWorld.rMax() + eAbs;
	double d{ 0 };
	World w1{ PreWorld };
	World w2{ PreWorld };
	StepVerletStep(w1, h);
	StepVerletStep(w2, h / 2);
	StepVerletStep(w2, h / 2);
	d = ErrCoe*DeltaPos(w1, w2);

	//Change step length and Estimated err
	if (d<e / 10 || d >= e) {
		while (d < e / 10 && h<hMax) {
			h *= 2;
			w1 = PreWorld;
			w2 = PreWorld;
			StepVerletStep(w1, h);
			StepVerletStep(w2, h / 2);
			StepVerletStep(w2, h / 2);
			d = ErrCoe*DeltaPos(w1, w2);

			cout << "step length doubled, now h=" << h << endl;
		}
		//！距离过近会造成数值精确度严重下降，能量不守恒
		while (d >= e) {
			h /= 2;
			w1 = PreWorld;
			w2 = PreWorld;
			StepVerletStep(w1, h);
			StepVerletStep(w2, h / 2);
			StepVerletStep(w2, h / 2);
			d = ErrCoe*DeltaPos(w1, w2);
			cout << "step length halfed, now h=" << h << endl;
		}
	}
	EstimatedErr = d;

	//Update PreWorld
	PreWorld = w2;
	DetectBorder(bound);
}

void Solver::StepRK4Ex() {
	const double ErrCoe = 16.0 / 15;

	double e = eRel*PreWorld.rMax() + eAbs;
	double d{ 0 };
	World w1{ PreWorld };
	World w2{ PreWorld };
	StepRK4ExStep(w1, h);
	StepRK4ExStep(w2, h/2);
	StepRK4ExStep(w2, h/2);
	d = ErrCoe*DeltaPos(w1, w2);

	//Change step length and Estimated err
	if (d<e / 10 || d >= e) {
		while (d < e / 10 && h<hMax) {
			h *= 2;
			w1 = PreWorld;
			w2 = PreWorld;
			StepRK4ExStep(w1, h);
			StepRK4ExStep(w2, h / 2);
			StepRK4ExStep(w2, h / 2);
			d = ErrCoe*DeltaPos(w1, w2);

			cout << "step length doubled, now h=" << h << endl;
		}
		//！距离过近会造成数值精确度严重下降，能量不守恒
		while (d >= e) {
			h /= 2;
			w1 = PreWorld;
			w2 = PreWorld;
			StepRK4ExStep(w1, h);
			StepRK4ExStep(w2, h / 2);
			StepRK4ExStep(w2, h / 2);
			d = ErrCoe*DeltaPos(w1, w2);

			cout << "step length halfed, now h=" << h << endl;
		}
	}
	EstimatedErr = d;

	//Update PreWorld
	PreWorld = w2;
	DetectBorder(bound);
}

void Solver::StepRK4Im()
{
	//Estimate err
	//this coefficient comes from err esimation
	const int ErrCoe = 2;

	double e = eRel*PreWorld.rMax() + eAbs;
	double d{ 0 };
	World w1{ PreWorld };
	World w2{ PreWorld };

	//Implicit solution
	StepRK4ImStep(w1, h);
	StepRK4ImStep(w2, h / 2);
	StepRK4ImStep(w2, h / 2);
	d = ErrCoe*DeltaPos(w1, w2);

	//Change step length and Estimated err
	if (d<e / 10 || d>e) {
		while (d < e / 10 && h<hMax) {
			h *= 2;
			w1 = PreWorld;
			w2 = PreWorld;
			StepRK4ImStep(w1, h);
			StepRK4ImStep(StepRK4ImStep(w2, h / 2), h / 2);
			d = ErrCoe*DeltaPos(w1, w2);

			cout << "step length doubled, now h=" << h << endl;
		}
		//！距离过近会造成数值精确度严重下降，能量不守恒
		while (d >= e) {
			h /= 2;
			w1 = PreWorld;
			w2 = PreWorld;
			StepRK4ImStep(w1, h);
			StepRK4ImStep(StepRK4ImStep(w2, h / 2), h / 2);
			d = ErrCoe*DeltaPos(w1, w2);

			cout << "step length halfed, now h=" << h << endl;
		}
	}
	EstimatedErr = d;

	//Update PreWorld
	PreWorld = w2;
	DetectBorder(bound);
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
void World::RandomCreate(int n) {
	double x, y, z, vx, vy, vz, mass;
	int id = 0;
	int ExpectedrMax = 5000;
	random_device rd;  // 将用于为随机数引擎获得种子
	std::mt19937 gen(rd()); // 以播种标准 mersenne_twister_engine
	// max r=2000, can be modified
	std::uniform_int_distribution<> dis(10, 2 * ExpectedrMax);
	
	while (id<n) {
		id++;
		mass = 1;
		// 用 dis 变换 gen 所生成的随机 unsigned int 到 [10, 2000] 中的 int
		x = dis(gen) - ExpectedrMax; y = dis(gen) - ExpectedrMax; z = dis(gen) - ExpectedrMax;
		vx = double(dis(gen) - ExpectedrMax) / ExpectedrMax * 20;
		vy = double(dis(gen) - ExpectedrMax) / ExpectedrMax * 20;
		vz = double(dis(gen) - ExpectedrMax) / ExpectedrMax * 20;
		MassPoint p(id, Point3d{ x,y,z }, Point3d{ vx,vy,vz }, mass);
		AddPlanet(p);
	}
}

void World::AddFromFile(ifstream &ifs)
{
	int id;
	double m, r, d, e, T;
	string name;
	double x, y, z, vx, vy, vz;
	while (ifs) {
		ifs >> id >> name >> m >> r >> d >> e >> T;
		vz = z = 0;
		x = d; y = 0;
		vx = (e > EPS0) ? sqrt(G*SUNMASS / (d*(1 + 1 / (e*e)))) : 0;
		vy = sqrt(G*SUNMASS / (d*(1 + e*e)));
		MassPoint p{ id,Point3d{x,y,z},Point3d{vx,vy,vz},m };
		AddPlanet(p);
	}
	ifs.close();
}

double World::CalcEnergy()
{
	double energy{ 0 };
	for (auto i = Planets.begin(); i != Planets.end(); i++) {
		//the sun energy
		energy +=
			-(G*SunMass*i->second.mass) / Norm(i->second.pos);
		//other planets' gravity
		for (auto j = Planets.begin(); j != Planets.end(); j++) {
			if (i->first != j->first) {
				energy +=
					-(G*i->second.mass*j->second.mass)
					/ Norm(i->second.pos - j->second.pos) / 2;
			}
		}
		//kinetic energy
		energy += 1.0 / 2 * i->second.mass*Norm(i->second.vel)*Norm(i->second.vel);
	}
	return energy;
}

dVel World::CalcdPos()
{
	dVel dp;
	for (auto i = Planets.begin(); i != Planets.end(); i++) { dp[i->first] = i->second.vel; }
	return dp;
}

int main()
{
	//create world
	World m_World{ SUNMASS };
	ifstream solar{ "twobody.txt" };
	m_World.AddFromFile(solar);

	//create solver
	Solver m_Solver{ METHOD_VERLET,BOUND_NONE,m_World };
	m_Solver.ChangeAbsErr(0.0001);
	m_Solver.ChangeRelErr(0.000001);
	m_Solver.SetIniEnergy(m_Solver.GetWorld().CalcEnergy());
	ofstream odata{ "output.txt" };
	ofstream oanalysis{ "analysis.txt" };
	odata << "Result:" << endl;
	int intervalInfo{ 50 };
	int intervalAna{ 2500 };
	int total{ 100000 };

	//Solve
	for (int i = 0; i < total && m_Solver.GetWorld().Planets.size()>0; i++) { 
		m_Solver.Step();
		if (i % intervalAna == 0)m_Solver.PrintAnalysis(oanalysis);
		if (i % intervalInfo == 0){
			m_Solver.PrintDetails(odata);
			odata << m_Solver.GetWorld() << endl;
		}

		if (i % intervalAna == 0)cout << "Step " << i << " out of" << total << endl;
	}
	return 0;
}
