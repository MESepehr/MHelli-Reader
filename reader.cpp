#include <QBuffer>
#include <QByteArray>
#include <QDebug>
#include <QFile>
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPainter>
#include <QVariant>
#include <QtGlobal>

#include <zbar.h>

#include "object.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <map>
#include <set>

using namespace std;

#define length(P) sqrt((P).x() * (P).x() + (P).y() * (P).y())

// #define _DEBUG_
// #define _LEVEL_1
// #define _LEVEL_2
// #define _LEVEL_3
// #define _LEVEL_Q

QImage qImage;
bool checked[1200][1200];
unsigned char image[1200][1200];
unsigned char imageOptions[1200][1200];
QImage sheet;
/** I یک هزارم طول چپ با راست صفحه و مقدار مثبت
J یک هزارم ارتفاع بالا به پایین صفحه با مقدار مثبت
O نقطه ی بالا سمت چپ صفحه و معادل نقطه ی f3
*/
QPointF I, J, O;
int OPTIONS;
int answers[300];
QList<QPoint> locations;

/**f1 top right
  f2 down righ
  f3 top left
  f4 down left
*/
Object *f1, *f2, *f3, *f4;

Object *detectObject(int x, int y, int W, int H) {
  if (checked[x][y])
    return new Object();
  if (!image[x][y])
    return new Object();
  Object *o = new Object();
  pair<void *, pair<int, int>> *S, *t;
  S = new pair<void *, pair<int, int>>(NULL, make_pair(x, y));
  checked[x][y] = true;
  while (S != NULL) {
    x = S->second.first;
    y = S->second.second;
    o->addPoint(QPoint(x, y), image[x][y]);
    t = S;
    S = (pair<void *, pair<int, int>> *)S->first;
    delete t;
    if (x + 1 < W && image[x + 1][y] && !checked[x + 1][y]) {
      S = new pair<void *, pair<int, int>>(S, make_pair(x + 1, y));
      checked[x + 1][y] = true;
    }
    if (x - 1 >= 0 && image[x - 1][y] && !checked[x - 1][y]) {
      S = new pair<void *, pair<int, int>>(S, make_pair(x - 1, y));
      checked[x - 1][y] = true;
    }
    if (y + 1 < H && image[x][y + 1] && !checked[x][y + 1]) {
      S = new pair<void *, pair<int, int>>(S, make_pair(x, y + 1));
      checked[x][y + 1] = true;
    }
    if (y - 1 >= 0 && image[x][y - 1] && !checked[x][y - 1]) {
      S = new pair<void *, pair<int, int>>(S, make_pair(x, y - 1));
      checked[x][y - 1] = true;
    }
    if (x + 1 < W && y + 1 < H && image[x + 1][y + 1] &&
        !checked[x + 1][y + 1]) {
      S = new pair<void *, pair<int, int>>(S, make_pair(x + 1, y + 1));
      checked[x + 1][y + 1] = true;
    }
    if (x + 1 < W && y - 1 >= 0 && image[x + 1][y - 1] &&
        !checked[x + 1][y - 1]) {
      S = new pair<void *, pair<int, int>>(S, make_pair(x + 1, y - 1));
      checked[x + 1][y - 1] = true;
    }
    if (x - 1 >= 0 && y + 1 < H && image[x - 1][y + 1] &&
        !checked[x - 1][y + 1]) {
      S = new pair<void *, pair<int, int>>(S, make_pair(x - 1, y + 1));
      checked[x - 1][y + 1] = true;
    }
    if (x - 1 >= 0 && y - 1 >= 0 && image[x - 1][y - 1] &&
        !checked[x - 1][y - 1]) {
      S = new pair<void *, pair<int, int>>(S, make_pair(x - 1, y - 1));
      checked[x - 1][y - 1] = true;
    }
  }
  return o;
}

bool findFlags() {
  QVector<Object *> objects;
  f1 = f2 = f3 = f4 = NULL;
  int width = sheet.width();
  int height = sheet.height();
  /**وسط صفحهی ما. میشه ۴۰۰ در حدود ۵۶۰*/
  QPoint origin(width / 2, height / 2);

  //کل صفحه رو بگرد
  for (int i = 0; i < width; i++) {
    for (int j = 0; j < height; j++) {
      if (checked[i][j])
        continue;
        //دایره هارو پیدا کن
      Object *o = detectObject(i, j, sheet.width(), sheet.height());
      if (o->surface() < 10) {
        //اگر دایره از ۱۰ پیکسل کوچیکتر بود بنداز دور
        //اگر صفحه به اندازه ۸۰۰ پیکسل طول داشته باشه (horizontal) 
        //  دایره های گوشه های صفحه میشن حدود ۲۰ پیکسل
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
      // باز چک می کنه اگر شعای دایره ها تو این بازه ها بودن بندازه دور. شعاع دایره باید در حدود ۱۰ پیکسل باشد
      continue;
    float sigma = o->sigma();
    if (sigma > 3)
      continue;

    /**وسط نقطه ی فعلی ما در حلقه ی اول*/
    QPoint center = o->center();
    int counter = 0;
    foreach (Object *o2, objects)
    //چک می کنه ببینه نقطه جدید ازنقطه ما راست تر و پایین تره؟
      if (length(o2->center() - origin) > length(center - origin))
        counter++;
    if (counter > 40)
      continue;//اگر ۴۰ نقطه ازش راست تر و پایین تر بودن میریم سراغ یه نقطه دیگه
    foreach (Object *o2, objects) {
      if (o2 == o)
        continue;
      if (length(o2->center() - center) > 4)
        continue;// اگر نقطه جدید از نقطه ما ۴ پیکسل دورتر بود یه نقطه نزدیکتر پیدا کنه
      if (o2->radius() < 2.12)
        continue;//اگر شعاعش از ۲ پیکسل کمتر بود بیخیال بشه
      if (o2->radius() > 8)
        continue;// اگر شعا از ۸ پیکسل بیشتر بود باز بی خیال بشه
      float sigma = o2->sigma();
      if (sigma > 3)
        continue;// سیگما نمی تونم چیه
      candidates << o;//این داره میگه اگه این نقطه چون نقطه های کوچیک با شعای کم و در اطرافش داشته، بیفته توی این لیست
    }
  }
  foreach (Object *o1, candidates) {
    /**مختصات نقطه ما می افته نسبت به وسط صفحه*/
    QPoint p1 = o1->center() - origin;
    foreach (Object *o2, candidates) {
      if (o2 == o1)
        continue;
        /**نقطه دوم می افته نسبت به مرکز صفحه*/
      QPoint p2 = o2->center() - origin;
      if (length(p1 - p2) < height / 2)
        continue;//اگر اختلاف فاصله نقطه های ما از نصف صفحه (یه عدد واضح) کمتر باشه، نقطه های نشانه ما نیست پس برو نقطه بعدی
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
    if (o->radius() > 22 || o->radius() < 6)
      continue;
    if (o->sigma() > 5)
      continue;
    if (length(o->center() - e1) < 100 and
        (!f3 || length(o->center() - e1) < length(f3->center() - e1)) and
        f3 != f1 and f3 != f2)
      f3 = o;
    if (length(o->center() - e2) < 100 and
        (!f4 || length(o->center() - e2) < length(f4->center() - e2)) and
        f4 != f1 and f4 != f2)
      f4 = o;
  }

  if (!f3 || !f4)
    return false;
    //میانگین ارتفاه بین نقطه ها
  verticalBorder =
      (f2->center() - f1->center() + f4->center() - f3->center()) / 2;
      //میانگین طول بین نقطه ها
  horizontalBorder =
      -(f3->center() - f1->center() + f4->center() - f2->center()) / 2;

  I = horizontalBorder / 1000;
  J = verticalBorder / 1000;
  O = f3->center();
  return true;
}
//QImage yek format ax baraye dastresi daghigh be pixel haye ye image ast dar c++
unsigned long long readBarcode(bool wide) {
  zbar::ImageScanner scanner;
  scanner.set_config(zbar::ZBAR_NONE, zbar::ZBAR_CFG_ENABLE, 1);
  scanner.set_config(zbar::ZBAR_NONE, zbar::ZBAR_CFG_POSITION, 1);
  QByteArray data;
  QBuffer buffer(&data);
  QImage croppedImage;
  if (I == QPointF() or wide)
    croppedImage =
        qImage.copy(QRect(0, 0, qImage.width(), qImage.height() / 5));
  else
    croppedImage = qImage.copy(QRectF(O + 25 * I + 5 * J, O + 300 * I + 180 * J)
                                   .toRect()
                                   .normalized());
  croppedImage.save(&buffer, "jpeg");
#ifdef _LEVEL_Q
  croppedImage.save("/tmp/b.jpg", "jpeg");
#endif
  zbar::Image zImage(croppedImage.width(), croppedImage.height(), "JPEG",
                     data.constData(), data.count());
  zbar::Image zImage2 = zImage.convert(
      ((unsigned long)('Y')) | ((unsigned long)('8') << 8) |
      ((unsigned long)('0') << 16) | ((unsigned long)('0') << 24));
  int n = scanner.scan(zImage2);
  if (n > 0) {
    for (int i = 0; i < zImage2.symbol_begin()->get_location_size(); i++)
      locations.append(QPoint(zImage2.symbol_begin()->get_location_x(i),
                              zImage2.symbol_begin()->get_location_y(i)));
    return QString::fromStdString(zImage2.symbol_begin()->get_data())
        .toULongLong();
  }
  return 0;
}

bool readAnswers120() {
  if (not f1 or not f2 or not f3 or not f4)
    return false;
  for (unsigned i = 0; i < 120; i++)
    answers[i] = 0;
  QPointF base(O + 115 * I + 315 * J);
  //andazeye gozine ha
  float c = 220, w = 32, h = 22.1;
  QList<unsigned> weights;
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 30; j++) {
      unsigned long mid = 0, var = 0;
      for (int k = 0; k < OPTIONS; k++) {
        //be tedade gozine ha
        QPointF localBase(base + i * c * I + j * h * J + k * w * I);
        int counter = 0;
        for (int x = 0; x < 25; x++) {
          for (int y = 0; y < 10; y++) {
            QPoint point((localBase + x * I + y * J).toPoint());
            counter += imageOptions[point.x()][point.y()];
          }
        }
        counter /= 25 * 10;
        //Miangine range hozine
        mid += counter;
        var += counter * counter * counter * counter;
      }
      mid /= OPTIONS;
      var = sqrt(sqrt(var - mid * mid));
      if (var > 31) {
        for (int k = 0; k < OPTIONS; k++) {
          QPointF localBase(base + i * c * I + j * h * J + k * w * I);
          unsigned counter = 0;
          for (int x = 0; x < 25; x++) {
            for (int y = 0; y < 10; y++) {
              QPoint point((localBase + x * I + y * J).toPoint());
              counter += imageOptions[point.x()][point.y()];
            }
          }
          counter = qMax(0u, counter);
          if (counter > (25 * 10) * 255 * 1 / 9 and
              counter / (25 * 10) > mid * 4 / 3) {
            weights << counter;
          }
        }
      }
    }
  }
  qSort(weights);
  if (weights.count() == 0)
    return true;
  unsigned median = weights[weights.count() * 2 / 3];
  foreach (unsigned weight, weights)
    if (weight < median * 1 / 2)
      weights.removeAll(weight);
  unsigned minWeight = weights[0] * 2 / 3;
#ifdef _DEBUG_
  qWarning() << minWeight;
#endif
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 30; j++) {
      unsigned long mid = 0, var = 0;
      int dd = 0;
      for (int k = 0; k < OPTIONS; k++) {
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
#ifdef _DEBUG_
      qWarning() << i * 30 + j + 1 << mid << var;
#endif
      if (var > 25) {
        for (int k = 0; k < OPTIONS; k++) {
          QPointF localBase(base + i * c * I + j * h * J + k * w * I);
          unsigned counter = 0;
          for (int x = 0; x < 25; x++) {
            for (int y = 0; y < 10; y++) {
              QPoint point((localBase + x * I + y * J).toPoint());
              counter += imageOptions[point.x()][point.y()];
            }
          }
          counter = qMax(0u, counter);
#ifdef _DEBUG_
          qWarning() << i * 30 + j + 1 << counter << counter / (25 * 10)
                     << mid * 4 / 3;
#endif
          if (counter > minWeight and counter / (25 * 10) > mid * 4 / 3) {
            dd |= 1 << k;
          }
        }
      }
      int sum = 0;
      for (int k = 0; k < OPTIONS; k++)
        sum += (dd & (1 << k)) / (1 << k);
      if (sum < (OPTIONS + 1) / 2)
      //i = sotun
      //j = satr
        answers[i * 30 + j] = dd;
    }
  }
  return true;
}

bool readAnswers300() {
  if (not f1 or not f2 or not f3 or not f4)
    return false;
  for (unsigned i = 0; i < 300; i++)//Shouldnt be 300?
    answers[i] = 0;
  QPointF base(O + 88 * I + 215 * J);
  float c = 151, w = 21.8, h = 15.2;
  QList<unsigned> weights;
  for (int i = 0; i < 6; i++) {
    for (int j = 0; j < 50; j++) {
      unsigned long mid = 0, var = 0;
      for (int k = 0; k < OPTIONS; k++) {
        QPointF localBase(base + i * c * I + j * h * J + k * w * I);
        int counter = 0;
        for (int x = 0; x < 13; x++) {
          for (int y = 0; y < 6; y++) {
            QPoint point((localBase + x * I + y * J).toPoint());
            counter += imageOptions[point.x()][point.y()];
          }
        }
        counter /= 13 * 6;
        mid += counter;
        var += counter * counter * counter * counter;
      }
      mid /= OPTIONS;
      var = sqrt(sqrt(var - mid * mid));
      if (var > 31) {
        for (int k = 0; k < OPTIONS; k++) {
          QPointF localBase(base + i * c * I + j * h * J + k * w * I);
          unsigned counter = 0;
          for (int x = 0; x < 13; x++) {
            for (int y = 0; y < 6; y++) {
              QPoint point((localBase + x * I + y * J).toPoint());
              counter += imageOptions[point.x()][point.y()];
            }
          }
          counter = qMax(0u, counter);
          if (counter > (13 * 6) * 255 * 1 / 9 and
              counter / (13 * 6) > mid * 4 / 3) {
            weights << counter;
          }
        }
      }
    }
  }
  qSort(weights);
  if (weights.count() == 0)
    return true;
  unsigned median = weights[weights.count() * 2 / 3];
  foreach (unsigned weight, weights)
    if (weight < median * 1 / 2)
      weights.removeAll(weight);
  unsigned minWeight = weights[0] * 2 / 3;
#ifdef _DEBUG_
  qWarning() << minWeight;
#endif
  for (int i = 0; i < 6; i++) {
    for (int j = 0; j < 50; j++) {
      unsigned long mid = 0, var = 0;
      int dd = 0;
      for (int k = 0; k < OPTIONS; k++) {
        QPointF localBase(base + i * c * I + j * h * J + k * w * I);
        int counter = 0;
        for (int x = 0; x < 13; x++) {
          for (int y = 0; y < 6; y++) {
            QPoint point((localBase + x * I + y * J).toPoint());
            counter += imageOptions[point.x()][point.y()];
          }
        }
        counter /= 13 * 6;
        mid += counter;
        var += counter * counter * counter * counter;
      }
      mid /= 4;
      var = sqrt(sqrt(var - mid * mid));
#ifdef _DEBUG_
      qWarning() << i * 50 + j + 1 << mid << var;
#endif
      if (var > 25) {
        for (int k = 0; k < OPTIONS; k++) {
          QPointF localBase(base + i * c * I + j * h * J + k * w * I);
          unsigned counter = 0;
          for (int x = 0; x < 13; x++) {
            for (int y = 0; y < 6; y++) {
              QPoint point((localBase + x * I + y * J).toPoint());
              counter += imageOptions[point.x()][point.y()];
            }
          }
          counter = qMax(0u, counter);
#ifdef _DEBUG_
          qWarning() << i * 50 + j + 1 << counter << counter / (13 * 6)
                     << mid * 4 / 3;
#endif
          if (counter > minWeight and counter / (13 * 6) > mid * 4 / 3) {
            dd |= 1 << k;
          }
        }
      }
      int sum = 0;
      for (int k = 0; k < OPTIONS; k++)
        sum += (dd & (1 << k)) / (1 << k);
      if (sum < (OPTIONS + 1) / 2)
        answers[i * 50 + j] = dd;
    }
  }
  return true;
}

/**دریافت فایل صفحه ی اسکن شده*/
char *_run(char *data, int size, int options, int type) {
  locations.clear();
  qImage = QImage::fromData((uchar *)data, size);
  OPTIONS = options;
  I = QPointF();
  J = QPointF();
  O = QPointF();
  bool answersStatus = false;

  if (qImage.width() > qImage.height()) {
    //Safhe amudi mishavad
    QTransform t;
    t.rotate(90);
    qImage = qImage.transformed(t, Qt::SmoothTransformation);
  }
  //Payna mishavad 800 pixel
  qImage = sheet = qImage.scaledToWidth(800, Qt::SmoothTransformation)
                       .convertToFormat(QImage::Format_ARGB32);

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
      //Meshki bashad( az miangine ghermez v sabze kol kamtar bashad va khotute abi dide nemisavad ) va ghatan khotute abi nabashad = 0 agar na rang karde ha
      //Harja siah bood mishe sefid abi ha mishan siah, kolan reverse image
      image[i][j] = (r + g) / 2 < MM and abs(r - b) + abs(r - g) < 130
                        ? 255 - qGray(sheet.pixel(i, j))
                        : 0;
      //Ehtemale sefid shodane paiini kamtare 
      imageOptions[i][j] =
          (r + g) / 2 < (MM * 3 / 2) and abs(r - b) + abs(r - g) < 130
              ? 255 - qGray(sheet.pixel(i, j))
              : 0;
    }

#ifdef _LEVEL_1
  {
    QPainter p(&sheet);
    for (int i = 0; i < sheet.width(); i++)
      for (int j = 0; j < sheet.height(); j++)
        if (image[i][j]) {
          p.setPen(QColor(255 - image[i][j], 255 - image[i][j],
                          255 - image[i][j], 255));
          p.drawPoint(i, j);
        };
    sheet.save("/tmp/1.jpg");
  }
#endif

  bool flags = findFlags();

#ifdef _LEVEL_2
  {
    QPainter p(&sheet);
    if (f1) {
      p.setBrush(QBrush(QColor(255, 255, 0, 127)));
      p.drawEllipse((QPointF)f1->center(), (qreal)f1->radius(),
                    (qreal)f1->radius());
    }

    if (f2) {
      p.setBrush(QBrush(QColor(255, 0, 0, 127)));
      p.drawEllipse((QPointF)f2->center(), (qreal)f2->radius(),
                    (qreal)f2->radius());
    }

    if (f3) {
      p.setBrush(QBrush(QColor(0, 255, 0, 127)));
      p.drawEllipse((QPointF)f3->center(), (qreal)f3->radius(),
                    (qreal)f3->radius());
    }

    if (f4) {
      p.setBrush(QBrush(QColor(255, 0, 255, 127)));
      p.drawEllipse((QPointF)f4->center(), (qreal)f4->radius(),
                    (qreal)f4->radius());
    }

    p.setBrush(QBrush(QColor(255, 0, 0, 0)));
    p.drawEllipse(QPointF(O + 57 * I + 110 * J), 5, 5);
    p.drawEllipse(QPointF(O + 57 * I + 110 * J + 30 * 22.3 * I), 5, 5);
    p.drawEllipse(QPointF(O + 57 * I + 110 * J + 3 * 18 * J), 5, 5);
    p.drawEllipse(QPointF(O + 57 * I + 110 * J + 30 * 22.3 * I + 3 * 18 * J), 5,
                  5);

    p.setBrush(QBrush(QColor(0, 255, 255, 127)));
    p.end();
    sheet.save("/tmp/2.jpg");
  }
#endif

  unsigned long long registrationID = readBarcode(false);
  if (not registrationID)
      registrationID = readBarcode(true);

  if (!flags) {
    QJsonObject result;
    result["error"] = "flags";
    QJsonArray l;
    foreach (QPoint point, locations) {
      QJsonArray p;
      p.append(point.x());
      p.append(point.y());
      l.append(p);
    }
    result["locations"] = l;
    if (registrationID)
      result["id"] = QString::number(registrationID);
    QString json(QJsonDocument(result).toJson(QJsonDocument::Compact));
    char *d = new char[json.count() + 1];
    qstrncpy(d, json.toStdString().c_str(), json.count() + 1);
    return d;
  }

  switch (type) {
  case 14:
    answersStatus = readAnswers300();
    break;
  case 15:
    answersStatus = readAnswers120();
    break;
  }
#ifdef _LEVEL_3
  {
    QPainter p(&sheet);
    p.setPen(QColor(0, 0, 0));
    if (type == 14) {
      QPointF base(O + 88 * I + 215 * J);
      float c = 151, w = 21.8, h = 15.2;

      for (int i = 0; i < 6; i++) {
        for (int j = 0; j < 50; j++) {
          for (int k = 0; k < 4; k++) {
            QPointF localBase(base + i * c * I + j * h * J + k * w * I);

            p.setPen(answers[i * 50 + j] & 1 << k ? QColor(0, 255, 0, 40)
                                                  : QColor(0, 0, 0, 40));
            for (int i = -2; i < 15; i++)
              for (int j = -2; j < 8; j++)
                p.drawPoint(localBase + i * I + j * J);
          }
        }
      }
    } else {
      QPointF base(O + 115 * I + 315 * J);
      float c = 220, w = 32, h = 22.1;

      for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 30; j++) {
          for (int k = 0; k < 4; k++) {
            QPointF localBase(base + i * c * I + j * h * J + k * w * I);

            p.setPen(answers[i * 30 + j] & 1 << k ? QColor(0, 255, 0, 40)
                                                  : QColor(0, 0, 0, 40));
            for (int i = -2; i < 27; i++)
              for (int j = -2; j < 12; j++)
                p.drawPoint(localBase + i * I + j * J);
          }
        }
      }
    }
    p.end();
    sheet.save("/tmp/4.jpg");
  }
#endif

  QJsonObject result;
  if (answersStatus) {
    QJsonArray A;
    for (int i = 0; i < (type == 15 ? 120 : 300); i++) {
      A.append(answers[i]);
    }
    result["answers"] = A;
  }
  if (registrationID)
    result["id"] = QString::number(registrationID);

  QString json(QJsonDocument(result).toJson(QJsonDocument::Compact));
  char *d = new char[json.count() + 1];
  qstrncpy(d, json.toStdString().c_str(), json.count() + 1);
  return d;
}

extern "C" {
char *run(char *image, int size, int options, int type) {
  return _run(image, size, options, type);
}
void freeString(char *ptr) { delete ptr; }
}
