#include <QVariant>
#include <QFile>
#include <QImage>
#include <QPainter>
#include <QDebug>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QByteArray>

#include "object.h"
#include <cmath>
#include <map>
#include <set>
#include <algorithm>
#include <cassert>

using namespace std;

#define length(P) sqrt((P).x() * (P).x() + (P).y() * (P).y())

#define INTERVAL 80

//#define _LEVEL_0
#define _LEVEL_1
#define _LEVEL_2
#define _LEVEL_3
#define _LEVEL_4
// #define _LEVEL_5


bool checked[1200][1200];
unsigned char image[1200][1200];
unsigned char imageOptions[1200][1200];
QImage sheet;
QPointF I, J, O;
QPointF b[4];
int answers[120];
bool barcodeTable[90];

Object *f1, *f2, *f3, *f4;

Object* detectObject(int x, int y, int W, int H) {
    if (checked[x][y])
        return new Object();
    if (!image[x][y])
        return new Object();
    Object *o = new Object();
    pair<void*, pair<int, int> > *S, *t;
    S = new pair<void*, pair<int, int> >(NULL, make_pair(x, y));
    checked[x][y] = true;
    while (S != NULL) {
        x = S->second.first;
        y = S->second.second;
        o->addPoint(QPoint(x, y), image[x][y]);
        t = S;
        S = (pair<void*, pair<int, int> >*)S->first;
        delete t;
        if (x + 1 < W && image[x + 1][y] && !checked[x + 1][y]) {
            S = new pair<void*, pair<int, int> >(S, make_pair(x + 1, y));
            checked[x + 1][y] = true;
        }
        if (x - 1 >= 0 && image[x - 1][y] && !checked[x - 1][y]) {
            S = new pair<void*, pair<int, int> >(S, make_pair(x - 1, y));
            checked[x - 1][y] = true;
        }
        if (y + 1 < H && image[x][y + 1] && !checked[x][y + 1]) {
            S = new pair<void*, pair<int, int> >(S, make_pair(x, y + 1));
            checked[x][y + 1] = true;
        }
        if (y - 1 >= 0 && image[x][y - 1] && !checked[x][y - 1]) {
            S = new pair<void*, pair<int, int> >(S, make_pair(x, y - 1));
            checked[x][y - 1] = true;
        }
        if (x + 1 < W && y + 1 < H && image[x + 1][y + 1] && !checked[x + 1][y + 1]) {
            S = new pair<void*, pair<int, int> >(S, make_pair(x + 1, y + 1));
            checked[x + 1][y + 1] = true;
        }
        if (x + 1 < W && y - 1 >= 0 && image[x + 1][y - 1] && !checked[x + 1][y - 1]) {
            S = new pair<void*, pair<int, int> >(S, make_pair(x + 1, y - 1));
            checked[x + 1][y - 1] = true;
        }
        if (x - 1 >= 0 && y + 1 < H && image[x - 1][y + 1] && !checked[x - 1][y + 1]) {
            S = new pair<void*, pair<int, int> >(S, make_pair(x - 1, y + 1));
            checked[x - 1][y + 1] = true;
        }
        if (x - 1 >= 0 && y - 1 >= 0 && image[x - 1][y - 1] && !checked[x - 1][y - 1]) {
            S = new pair<void*, pair<int, int> >(S, make_pair(x - 1, y - 1));
            checked[x - 1][y - 1] = true;
        }
    }
    return o;
}

bool findFlags() {
    f1 = f2 = f3 = f4 = NULL;
    QVector<Object*> objects;
    int width = sheet.width();
    int height = sheet.height();
    QPoint origin(width / 2, height / 2);

    for (int i = 0; i < width; i++) {
        for (int j = 0; j < height; j++) {
            if (checked[i][j])
                continue;
            Object *o = detectObject(i, j, sheet.width(), sheet.height());
            if (o->surface() < 10) {
                delete o;
                continue;
            }
            objects << o;
        }
    }


    QList<Object *> candidates;
    foreach (Object *o, objects) {
        float radius = o->radius();
        if (radius > 20 || radius < 5)
            continue;
        float sigma = o->sigma();
        if (sigma > 3)
            continue;

        QPoint center = o->center();
        int counter = 0;
        foreach (Object *o2, objects)
            if (length(o2->center() - origin) > length(center - origin))
                counter++;
        if (counter > 40)
            continue;
        foreach (Object *o2, objects) {
            float sigma = o2->sigma();
            if (o2 == o)
                continue;
            if (length(o2->center() - center) > 4)
                continue;
            if (o2->radius() < 3)
                continue;
            if (o2->radius() > 8)
                continue;
            if (sigma > 3)
                continue;
            candidates << o;
        }
    }
    foreach (Object *o1, candidates) {
        QPoint p1 = o1->center() - origin;
        foreach (Object *o2, candidates) {
            if (o2 == o1)
                continue;
            QPoint p2 = o2->center() - origin;
            if (length(p1 - p2) < height / 2)
                continue;
            if (p1.x() * p2.y() - p1.y() * p2.x() < 0)
                continue;
            f1 = o1;
            f2 = o2;
            break;
        }
    }

    if (!f1 || !f2)
        return false;

    QTransform t;
    t.rotate(-90);
    QPointF verticalBorder(f2->center() - f1->center());
    QPointF horizontalBorder(t.map(verticalBorder));
    horizontalBorder *= .684;

    QPointF e1(f1->center() - horizontalBorder);
    QPointF e2(f2->center() - horizontalBorder);

    foreach (Object *o, objects) {
        if (o->radius() > 22 || o->radius() < 7)
            continue;
        if (o->sigma() > 5)
            continue;
        if (length(o->center() - e1) < 100 and (!f3 || length(o->center() - e1) < length(f3->center() - e1)) and f3 != f1 and f3 != f2)
            f3 = o;
        if (length(o->center() - e2) < 100 and (!f4 || length(o->center() - e2) < length(f4->center() - e2)) and f4 != f1 and f4 != f2)
            f4 = o;
    }

    if (!f3 || !f4)
        return false;
    verticalBorder = (f2->center() - f1->center() + f4->center() - f3->center()) / 2;
    horizontalBorder = -(f3->center() - f1->center() + f4->center() - f2->center()) / 2;

    I = horizontalBorder / 1000;
    J = verticalBorder / 1000;
    O = f3->center();

    QPointF base[4] = {QPointF(O + 63 * I + 110 * J),
        QPointF(O + 63 * I + 110 * J + 30 * 21.8 * I),
        QPointF(O + 63 * I + 110 * J + 3 * 18 * J),
        QPointF(O + 63 * I + 110 * J + 30 * 21.8 * I + 3 * 18 * J)};
    float l[4] = {-1, -1, -1, -1};
    foreach (Object *o, objects) {
        for (int i = 0; i < 4; i++) {
            QPointF p(o->center() - base[i]);
            float X = p.x() * I.x() + p.y() * I.y();
            if (i % 2)
                X = -X;
            float Y = p.x() * J.x() + p.y() * J.y();
            if (i > 1)
                Y = -Y;
            if (o->sigma() < 7 and X > 0 and Y > 0) {
                if (l[i] < 0 or X * X * X + Y * Y * Y < l[i]) {
                    b[i] = o->center();
                    l[i] = X * X * X + Y * Y * Y;
                }
            }
        }
    }

    return true;
}

bool readBarcode() {
    float w = length(b[1] - b[0]) / 31, h = length(b[2] - b[0]) / 2;
    QPointF I = (b[1] - b[0]) / length(b[1] - b[0]);
    QPointF J = (b[2] - b[0]) / length(b[2] - b[0]);

    unsigned long long mid = 0, cc = 0;
    for (int i = 1; i < 31; i++) {
        for (int j = 0; j < 3; j++) {
            QPointF localBase(b[1] - w * i * I + h * j * J);
            int counter = 0;
            for (float x = -w / 2; x <= w / 2; x++) {
                for (float y = -h / 2; y <= h / 2; y++) {
                    QPoint point((localBase + x * I + y * J).toPoint());
                    cc++;
                    mid += image[point.x()][point.y()];
                }
            }
        }
    }
    mid /= cc;
    for (int i = 1; i < 31; i++) {
        for (int j = 0; j < 3; j++) {
            QPointF localBase(b[1] - w * i * I + h * j * J);
            int counter = 0;
            for (float x = -w / 2; x <= w / 2; x++) {
                for (float y = -h / 2; y <= h / 2; y++) {
                    QPoint point((localBase + x * I + y * J).toPoint());
                    counter += image[point.x()][point.y()];
                }
            }
            barcodeTable[i - 1 + j * 30] = counter > w * h * mid;
        }
    }
    return true;
}

bool readAnswers() {
    if (not f1 or not f2 or not f3 or not f4)
        return false;
    QPointF base(O + 115 * I + 315 * J);
    float c = 220, w = 32, h = 22.1;
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 30; j++) {
            unsigned long mid = 0, var = 0;
            int dd = 0;
            for (int k = 0; k < 4; k++) {
                QPointF localBase(base + i * c * I + j * h * J + k * w * I);
                int counter = 0;
                for (int x = 0; x < 25; x++) {
                    for (int y = 0; y < 10; y++) {
                        QPoint point((localBase + x * I + y * J).toPoint());
                        counter += imageOptions[point.x()][point.y()];
                    }
                }
                counter /= 25 * 10;
                mid += counter;
                var += counter * counter * counter * counter;
            }
            mid /= 4;
            var = sqrt(sqrt(var - mid * mid));
            if (var > 31) {
                for (int k = 0; k < 4; k++) {
                    QPointF localBase(base + i * c * I + j * h * J + k * w * I);
                    int counter = 0;
                    for (int x = 0; x < 25; x++) {
                        for (int y = 0; y < 10; y++) {
                            QPoint point((localBase + x * I + y * J).toPoint());
                            counter += imageOptions[point.x()][point.y()];
                        }
                    }
                    counter = qMax(0, counter);
                    if (counter > (25 * 10) * 255 * 1 / 8 and counter / (25 * 10) > mid * 4 / 3) {
                        dd |= 1 << k;
                    }
                }
            }
            if ((dd & 1) + (dd & 2) / 2 + (dd & 4) / 4 + (dd & 8) / 8 < 3)
                answers[i * 30 + j] = dd;
        }
    }
    return true;
}

char *_run(char *data, int size) {
    QImage mImage = QImage::fromData((uchar *)data, size);
    bool answersStatus = false;

    unsigned long long registrationID = 0;
    int err = -1;
    bool barcode = false;
    if (mImage.width() > mImage.height()) {
        QTransform t;
        t.rotate(90);
        mImage = mImage.transformed(t, Qt::SmoothTransformation);
    }
    sheet = mImage.scaledToWidth(800, Qt::SmoothTransformation).convertToFormat(QImage::Format_ARGB32);

    // emit newStage(0, "Remove noises by color filters...", mImage);

    long long int MM = 0;
    for (int i = 0; i < sheet.width(); i++)
        for (int j = 0; j < sheet.height(); j++)
            MM += (qRed(sheet.pixel(i, j)) + qGreen(sheet.pixel(i, j))) / 2;
    MM /= sheet.width() * sheet.height();

    MM *= 5;
    MM /= 6;
    for (int i = 0; i < sheet.width(); i++)
        for (int j = 0; j < sheet.height(); j++) {
            checked[i][j] = false;
            int r = qRed(sheet.pixel(i, j));
            int g = qGreen(sheet.pixel(i, j));
            int b = qBlue(sheet.pixel(i, j));
            image[i][j] = (r + g) / 2 < MM and abs(r - b) + abs(r - g) < 100 ? 255 - qGray(sheet.pixel(i, j)) : 0;
            imageOptions[i][j] = (r + g) / 2 < (MM * 3 / 2) and abs(r - b) + abs(r - g) < 100 ? 255 - qGray(sheet.pixel(i, j)) : 0;
        }

#ifdef _LEVEL_1
    {
        QPainter p(&sheet);
        for (int i = 0; i < sheet.width(); i++)
            for (int j = 0; j < sheet.height(); j++)
                if (image[i][j])
                    p.drawPoint(i, j);
    }
#endif

    // emit newStage(1, "Detecting flags...", sheet.scaledToWidth(800));

    bool flags = findFlags();

#ifdef _LEVEL_2
    {
        QPainter p(&sheet);
        if (!f1);
        else {
            p.setBrush(QBrush(QColor(255, 255, 0, 127)));
            p.drawEllipse((QPointF)f1->center(), (qreal)f1->radius(), (qreal)f1->radius());
        }

        if (!f2);
        else {
            p.setBrush(QBrush(QColor(255, 0, 0, 127)));
            p.drawEllipse((QPointF)f2->center(), (qreal)f2->radius(), (qreal)f2->radius());
        }

        if (!f3);
        else {
            p.setBrush(QBrush(QColor(0, 255, 0, 127)));
            p.drawEllipse((QPointF)f3->center(), (qreal)f3->radius(), (qreal)f3->radius());
        }

        if (!f4);
        else {
            p.setBrush(QBrush(QColor(255, 0, 255, 127)));
            p.drawEllipse((QPointF)f4->center(), (qreal)f4->radius(), (qreal)f4->radius());
        }


        p.setBrush(QBrush(QColor(255, 0, 0, 0)));
        p.drawEllipse(QPointF(O + 63 * I + 110 * J), 5, 5);
        p.drawEllipse(QPointF(O + 63 * I + 110 * J + 30 * 22 * I), 5, 5);
        p.drawEllipse(QPointF(O + 63 * I + 110 * J + 3 * 18 * J), 5, 5);
        p.drawEllipse(QPointF(O + 63 * I + 110 * J + 30 * 22 * I + 3 * 18 * J), 5, 5);

        p.setBrush(QBrush(QColor(0, 255, 255, 127)));
        p.drawEllipse(b[0], 10, 10);
        p.drawEllipse(b[1], 10, 10);
        p.drawEllipse(b[2], 10, 10);
        p.drawEllipse(b[3], 10, 10);
        p.end();
    }
#endif

    if (!flags) {
        // error("Corrupted paper", QString::fromUtf8(
        //             "مشکل در خواندن علائم صفحه.") + QString::number(err));
        qWarning() << "E1";
        return NULL;
    }

    // emit newStage(2, "Reading barcode...", sheet.scaledToWidth(800));

    barcode = readBarcode();
#ifdef _LEVEL_3
    {
        QPainter p(&sheet);
        float w = length(b[1] - b[0]) / 31, h = length(b[2] - b[0]) / 2;
        QPointF I = (b[1] - b[0]) / length(b[1] - b[0]);
        QPointF J = (b[2] - b[0]) / length(b[2] - b[0]);

        for (int i = 1; i < 31; i++) {
            for (int j = 0; j < 3; j++) {
                QPointF localBase(b[1] - w * i * I + h * j * J);
                p.setPen(barcodeTable[i - 1 + j * 30] ? QColor(0, 255, 255, 137) : QColor(255, 255, 255, 187));
                for (float x = -w / 2; x <= w / 2; x++)
                    for (float y = -h / 2; y <= h / 2; y++)
                        p.drawPoint(localBase + x * I + y * J);
            }
        }
        p.end();
    }
#endif

    // emit newStage(3, "Reading answers...", sheet.scaledToWidth(800));
    if (!barcode) {
        // error("Corrupted paper", QString::fromUtf8(
        //             "مشکل در خواندن علائم بارکد.") + QString::number(err));
        qWarning() << "E2";
        return NULL;
    }


    // emit newStage(3, "Reading answers...", sheet.scaledToWidth(800));

    answersStatus = readAnswers();
#ifdef _LEVEL_4
    {
        QPainter p(&sheet);
        p.setPen(QColor(0, 0, 0));
        QPointF base(O + 115 * I + 315 * J);
        float c = 220, w = 32, h = 22.1;

        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 30; j++) {
                for (int k = 0; k < 4; k++) {
                    QPointF localBase(base + i * c * I + j * h * J + k * w * I);

                    p.setPen(answers[i * 30 + j] & 1 << k ? QColor(0, 255, 0, 60) : QColor(0, 0, 0, 60));
                    for (int i = -2; i < 27; i++)
                        for (int j = -2; j < 12; j++)
                            p.drawPoint(localBase + i * I + j * J);
                }
            }
        }
        p.end();
    }
#endif

    if (!answersStatus) {
        // error("Corrupted paper", QString::fromUtf8(
        //             "مشکل در خواندن علائم بارکد.") + QString::number(err));
        qWarning() << "E3";
        return NULL;
    }

    // emit newStage(4, "Processing Barcode...", sheet.scaledToWidth(800));

    if (barcode) {
        bool A[2][46];
        for (int i = 0; i < 45; i++) {
            A[0][i] = barcodeTable[i];
            A[1][i] = (barcodeTable + 45)[i];
        }
        A[0][45] = A[1][45] = 0;
        int toChange0 = 0, toChange1 = 0;
        for (int i = 0; i <= 5; i++) {
            bool p1 = false, p2 = false;
            for (int j = 0; j < 45; j++)
                if ((1 << i) & (j + 1)) {
                    p1 ^= A[0][j];
                    p2 ^= A[1][j];
                }
            if (p1)
                toChange0 += 1 << i;
            if (p2)
                toChange1 += 1 << i;
        }
        if (toChange0 <= 45 and toChange0 > 0) {
            A[0][toChange0 - 1] ^= 1;
        }
        if (toChange1 <= 45 and toChange1 > 0) {
            A[1][toChange1 - 1] ^= 1;
        }

        unsigned long long bid = 0;
        for (int i = 0, j = 0; i < 45; i++) {
            if (i + 1 == 1 || i + 1 == 2 || i + 1 == 4 || i + 1 == 8 || i + 1 == 16 || i + 1 == 32)
                continue;
            if (A[0][i] != A[1][i]) {
                if (toChange0 && toChange1) {
                    err = 200 + i;
                    barcode = false;
                }
                else if (toChange0)
                    bid += A[1][i] * (1llu << j);
                else if (toChange1)
                    bid += A[0][i] * (1llu << j);
                else {
                    err = 200 + i;
                    barcode = false;
                }
            }
            else
                bid += A[0][i] * (1llu << j);
            j++;
        }
        registrationID = bid;
        if (registrationID == 0)
            barcode = 0;
    }

    QJsonArray A;
    for (int i = 0; i < 120; i++) {
        A.append(answers[i]);
    }
    QJsonObject result;
    result["answers"] = A;
    result["id"] = QString::number(registrationID);

    QString json(QJsonDocument(result).toJson(QJsonDocument::Compact));
    char *d = new char[json.count() + 1];
    qstrncpy(d, json.toStdString().c_str(), json.count() + 1);
    return d;
}

extern "C" {
    char *run(char *image, int size) {return _run(image, size);}
    void freeString(char *ptr) {delete ptr;}
}
