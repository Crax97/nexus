/*
Nexus

Copyright(C) 2012 - Federico Ponchio
ISTI - Italian National Research Council - Visual Computing Lab

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License (http://www.gnu.org/licenses/gpl.txt)
for more details.
*/
#include <iostream>
#include "cone.h"

using namespace std;
using namespace vcg;
using namespace nx;

AnchoredCone3f::AnchoredCone3f() {
	scaledNormal = Point3f(0,0,0);
	frontAnchor = Point3f(0,0,0);
	backAnchor = Point3f(0,0,0);
}

bool AnchoredCone3f::Frontface(const Point3f &viewPoint) {
	Point3f d = frontAnchor - viewPoint; //Vector from viewPoint to frontAnchor.
	float f = - d * scaledNormal;
	if (f < 0.001 || f * f < d * d)
		return false;
	return true;
}

bool AnchoredCone3f::Backface(const Point3f &viewPoint) {
	Point3f d = backAnchor - viewPoint; //Vector from viewPoint to frontAnchor.
	float f = d * scaledNormal;
	if (f < 0.001 || f * f < d * d)
		return false;
	return true;
}

void AnchoredCone3f::AddNormals(vector<Point3f> &normal, vector<float> &area, float threshold) {
	assert(normal.size() == area.size());
	//compute average normal
	scaledNormal = Point3f(0,0,0);
	for(auto &n: normal)
		scaledNormal += n;
	scaledNormal.Normalize();

	//create 50 concentric rings around the average normal (scaledNormal)
	double distr[50];
	for(int k = 0; k < 50; k++)
		distr[k] = 0;
	double tot_area = 0;

	vector<float>::iterator j;
	vector<Point3f>::iterator i;
	for(i = normal.begin(), j = area.begin(); i != normal.end(); i++, j++) {
		int pos = (int)(49.0 * Angle(scaledNormal, *i)/M_PI);
		if(pos < 0) continue;
		assert(pos >=0 && pos < 50);
		distr[pos] += *j;
		tot_area += *j;
	}

	float tot = 0;
	int best;
	for(best = 0; best < 50; best++) {
		tot += (float)distr[best];
		if(tot > threshold * tot_area)
			break;
	}
	double alpha = M_PI * (best + 1) / 50;
	if(alpha >= M_PI/ 2 - 0.1)
		scaledNormal = Point3f(0,0,0);
	else
		scaledNormal /= cos(M_PI/2.0f - alpha);
}

void AnchoredCone3f::AddNormals(vector<Point3f> &normal, float threshold) {
	assert(normal.size() > 0);
	scaledNormal = Point3f(0,0,0);
	int count = 0;
	for(auto &n: normal) {
		if(n.Norm() < 0.00001) continue;
		n.Normalize();
		scaledNormal += n;
		count++;
	}
	scaledNormal /= count;
	float len = scaledNormal.Norm();
	if(len == 0) return;
	scaledNormal /= len;

	int distr[50];
	for(int k = 0; k < 50; k++)
		distr[k] =0;

	for(auto &n: normal) {
		int pos = (int)(49.0 * Angle(scaledNormal, n)/M_PI);
		distr[pos]++;
	}
	int tot = 0;
	int best;
	for(best = 0; best < 50; best++) {
		tot += distr[best];
		if(tot >= threshold * normal.size())
			break;
	}
	double alpha = M_PI * (best +1) / 50;
	if(alpha >= M_PI/ 2 - 0.1) {
		scaledNormal = Point3f(0,0,0);
	} else {
		scaledNormal /= cos(M_PI/2.0f - alpha);
	}
}

void AnchoredCone3f::AddAnchors(vector<Point3f> &anchors) {
	assert(anchors.size() > 0);
	frontAnchor = anchors[0];
	backAnchor = anchors[0];

	float fa = frontAnchor * scaledNormal;
	float fb = -backAnchor * scaledNormal;

	vector<Point3f>::iterator i;
	for(i = anchors.begin(); i != anchors.end(); i++) {
		Point3f &anchor = *i;
		float na = anchor * scaledNormal;
		if(na < fa) {
			frontAnchor = anchor;
			fa = na;
		}
		if(-na < fb) {
			backAnchor = anchor;
			fb = -na;
		}
	}
}

void Cone3s::Import(const AnchoredCone3f &c) {
	Point3f normal = c.scaledNormal;
	float len = normal.Norm();
	if(len > 0.001)
		normal /= len;

	for(int i = 0; i < 3; i++) {
		assert(normal[i] < 1.01 && normal[i] > -1.01);

		if(normal[i] > 1.0f) normal[i] = 1;
		if(normal[i] < -1.0f) normal[i] = -1;
	}
	n[0] = (short)(normal[0] * 32766);
	n[1] = (short)(normal[1] * 32766);
	n[2] = (short)(normal[2] * 32766);
	//i want to represent numbers from -10 to 10
	if(len > 10.0f) len = 10.0f;
	if(len < -10.0f) len = -10.0f;
	n[3] = (short)(len * 3276);
}


bool Cone3s::Backface(const vcg::Sphere3f &sphere,
					  const vcg::Point3f &view) const {
	vcg::Point3f norm(n[0]/32766.0f, n[1]/32766.0f, n[2]/32766.0f);
	vcg::Point3f d = (sphere.Center() - norm * sphere.Radius()) - view;
	norm *= n[3]/32766.0f;

	float f = d * norm;
	if (f < 0.001 || f * f < d * d)
		return false;
	return true;
}

bool Cone3s::Frontface(const vcg::Sphere3f &sphere,
					   const vcg::Point3f &view) const {
	vcg::Point3f norm(n[0]/32766.0f, n[1]/32766.0f, n[2]/32766.0f);
	vcg::Point3f d = (sphere.Center() + norm * sphere.Radius()) - view;
	norm *= n[3]/3276.0f;

	float f = -d * norm;
	if (f < 0.001 || f * f < d * d)
		return false;
	return true;
}
