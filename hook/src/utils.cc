#include <NickelHook.h>
#include <QObject>

#include "utils.h"

void printClass(QObject *view) {
  QString str = QString("");
  const QMetaObject *m = view->metaObject();

  while (m) {
    str.append("->").append(m->className());
    m = m->superClass();
  }

  nh_log("%s", qPrintable(str));
}

void printChild(QObject *view, int d) {
  QString name = QString(view->metaObject()->className());
  QObjectList children = view->children();

  for (int i = 0; i < children.size(); ++i) {
    QObject *child = children[i];
    QString str = QString(d, ' ');

    const QMetaObject *m = child->metaObject();
    str.append(m->className());
    m = m->superClass();

    while (m) {
      str.append("->").append(m->className());
      m = m->superClass();
    }

    nh_log("%s", qPrintable(str));
    printChild(children[i], ++d);
  }
}
