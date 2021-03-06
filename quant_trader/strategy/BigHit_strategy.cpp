#include "utility.h"
#include "../bar.h"
#include "../mql5_compatible.h"
#include "bighit_strategy.h"

#include <cfloat>
#include <QVariant>
#include <QTime>

BigHitStrategy::BigHitStrategy(const QString &id, const QString &instrumentID, const QString &time_frame, QObject *parent) :
    AbstractStrategy(id, instrumentID, time_frame, parent),
    biggestVolEver(false),
    bigHit(0)
{
    //
}

void BigHitStrategy::setParameter(const QVariant& param1, const QVariant& param2, const QVariant& param3,
                                  const QVariant& /*4*/ , const QVariant& /*5*/ , const QVariant& /*6*/ ,
                                  const QVariant& /*7*/ , const QVariant& /*8*/ , const QVariant& /*9*/)
{
    int deltaTime = param1.toInt();
    double deltaPrice = param2.toDouble();
    int timeOut = param3.toInt();

    setParameter(deltaTime, deltaPrice, timeOut);
}

void BigHitStrategy::setParameter(int deltaTime, double deltaPrice, int timeOut)
{
    dT = deltaTime;
    qDebug() << "deltaTime =" << dT;
    dP = deltaPrice;
    qDebug() << "deltaPrice =" << dP;
    tO = timeOut;
    qDebug() << "timeOut =" << tO;
}

void BigHitStrategy::onNewBar()
{
    if (barList->count() < 2) {
        return;
    }

    const int count = barList->count();
    const auto vol = barList->at(count - 1).volume;
    const auto lastTime = barList->at(count -1).time;
    int i = 0;
    for (; i < (count - 1); i++) {
        const auto barTime = barList->at(i).time;
        if (isTimeCloseEnouogh(barTime, lastTime, 5400) && (vol < barList->at(i).volume) && (barTime % 86400 != 32400)) {
            break;
        }
    }
    if (i == (count - 1)) {
        biggestVolEver = true;
        qDebug() << " ****** Found Biggest Volume!";
        expireTime = barList->at(count - 1).time + 60 + tO;
    } else {
        if (biggestVolEver && barList->at(count - 1).time + 60 > expireTime) {
            qDebug() << " ^^^^^^ Biggest Volume Expired!";
            biggestVolEver = false;
        }
    }
}

void BigHitStrategy::onNewTick(uint time, double lastPrice)
{
    if (!recentPrices.empty()) {
        while (!recentPrices.empty() && !isTimeCloseEnouogh(recentPrices.head().first, time, dT)) {
            recentPrices.dequeue();
        }

        if (!recentPrices.empty()) {
            if (recentPrices.head().second - lastPrice > dP) {
                qDebug() << " ------ bigHit sell!";
                bigHit = -1;
            } else if (lastPrice - recentPrices.head().second > dP) {
                qDebug() << " ------ bigHit buy!";
                bigHit = 1;
            } else {
                bigHit = 0;
            }
        } else {
            bigHit = 0;
        }
    } else {
        bigHit = 0;
    }

    if (biggestVolEver && (time < expireTime)) {
        if (bigHit > 0) {
            position = 1;
        } else if (bigHit < 0) {
            position = -1;
        }
    } else {
        if (position.is_initialized()) {
            if (position.value() > 0 && bigHit < 0) {
                position = 0;
            } else if (position.value() < 0 && bigHit > 0){
                position = 0;
            }
        }
    }
    recentPrices.enqueue(qMakePair(time, lastPrice));
}
