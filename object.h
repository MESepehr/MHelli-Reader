/*
 * Object.h
 *
 *  Created on: Sep 21, 2008
 *      Author: root
 */

#ifndef OBJECT_H_
#define OBJECT_H_

#include <QPoint>
#include <QTransform>
#include <map>

bool operator<(const QPoint &, const QPoint &);

class Object {
private:
	mutable bool done;

protected:
    std::map<QPoint, unsigned char> mPoints;
	mutable QPoint mCenter;
	mutable float mSurface;
	mutable float mDistance;
	mutable float mRadius;
	mutable float mSigma;

public:
	Object();
	virtual ~Object();

    const std::map<QPoint, unsigned char>& getPoints() const;
    void addPoint(QPoint, int);
	int surface() const;
	int distance() const;
	void transform(const QTransform&);
	QPoint center() const;
	float radius() const;
	float sigma() const;

	Object& operator =(const Object&);
	Object& operator +=(const Object&);
};

#endif /* OBJECT_H_ */
