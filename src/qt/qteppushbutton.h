#ifndef QTEPPUSHBUTTON_H
#define QTEPPUSHBUTTON_H
#include <QPushButton>
#include <QStyleOptionButton>
#include <QIcon>

class QtepPushButton : public QPushButton
{
public:
    explicit QtepPushButton(QWidget * parent = Q_NULLPTR);
    explicit QtepPushButton(const QString &text, QWidget *parent = Q_NULLPTR);

protected:
    void paintEvent(QPaintEvent *) Q_DECL_OVERRIDE;

private:
    void updateIcon(QStyleOptionButton &pushbutton);

private:
    bool m_iconCached;
    QIcon m_downIcon;
};

#endif // QTEPPUSHBUTTON_H
