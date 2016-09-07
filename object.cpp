
/*
 * Object.cpp
 *
 *  Created on: Sep 21, 2008
 *      Author: root
 */

#include "Object.h"
#include <QDebug>
#include <cmath>

using namespace std;

bool operator<(const QPoint &p1, const QPoint &p2) {
    return p1.x() < p2.x() or (p1.x() == p2.x() and p1.y() < p2.y());
}

Object::Object(): mCenter(0, 0), mSurface(-1), mDistance(-1), mRadius(-1), mSigma(-1) {
}

Object::~Object() {
    mPoints.clear();
}

const map<QPoint, unsigned char>& Object::getPoints() const {
    return mPoints;
}

void Object::addPoint(QPoint p, int value) {
    mPoints[p] = value;
    mCenter = QPoint(0, 0);
    mSurface = -1;
    mDistance = -1;
    mRadius = -1;
    mSigma = -1;
}

int Object::surface() const {
    if (mSurface < 0)
        mSurface = mPoints.size();
    return mSurface;
}

int Object::distance() const {
    if (mDistance < -1) {
        int l = mPoints.begin()->first.x(), r = mPoints.begin()->first.x(), t = mPoints.begin()->first.y(), d = mPoints.begin()->first.y();
        for (map<QPoint, unsigned char>::const_iterator i = mPoints.begin(); i != mPoints.end(); i++) {
            if (i->first.x() < l)
                l = i->first.x();
            if (i->first.x() > l)
                r = i->first.x();
            if (i->first.y() < t)
                t = i->first.y();
            if (i->first.y() > d)
                d = i->first.y();
        }
        mDistance = max(r - l, d - t);
    }
    return mDistance;
}

void Object::transform(const QTransform &t) {
    map<QPoint, unsigned char> m;
    for (map<QPoint, unsigned char>::iterator i = mPoints.begin(); i != mPoints.end(); i++)
        m[t.map(i->first)] = i->second;
    mPoints.clear();
    mPoints.insert(m.begin(), m.end());
    mCenter = QPoint(0, 0);
    mSurface = -1;
    mDistance = -1;
    mRadius = -1;
    mSigma = -1;
}

QPoint Object::center() const {
    if (mCenter == QPoint(0, 0)) {
        QPoint p(0, 0);
        int v = 0;
        for (map<QPoint, unsigned char>::const_iterator i = mPoints.begin(); i != mPoints.end(); i++) {
            p += i->first * i->second;
            v += i->second;
        }
        mCenter = p / v;
    }
    return mCenter;
}

#define LENGTH(P) hypot((P).x(), (P).y())

float Object::radius() const {
    if (mRadius < 0) {
        QPoint p(center());
        float r = .0f;
        int v = 0;
        for (map<QPoint, unsigned char>::const_iterator i = mPoints.begin(); i != mPoints.end(); i++) {
            r += LENGTH(p - i->first) * i->second;
            v += i->second;
        }
        mRadius = r / v;
    }
    return mRadius;
}

float Object::sigma() const {
    if (mSigma < 0) {
        QPoint p(center());
        float r = radius();
        float sigma = .0f;
        int v = 0;
        for (map<QPoint, unsigned char>::const_iterator i = mPoints.begin(); i != mPoints.end(); i++) {
            float f = LENGTH(p - i->first) - r;
            sigma += f * f * i->second;
            v += i->second;
        }
        mSigma = sigma / v;
    }
    return mSigma;
}

Object& Object::operator =(const Object& o) {
    mPoints.clear();
    mPoints.insert(o.getPoints().begin(), o.getPoints().end());
    mCenter = o.mCenter;
    mSurface = o.mSurface;
    mDistance = o.mDistance;
    mRadius = o.mRadius;
    mSigma = o.mSigma;
    return *this;
}

Object& Object::operator +=(const Object& o) {
    mPoints.insert(o.getPoints().begin(), o.getPoints().end());
    mCenter = QPoint(0, 0);
    mSurface = -1;
    mDistance = -1;
    mRadius = -1;
    mSigma = -1;
    return *this;
}
