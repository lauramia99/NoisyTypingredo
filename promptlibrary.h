#ifndef PROMPTLIBRARY_H
#define PROMPTLIBRARY_H

#include <QString>
#include <QVector>

struct PromptDefinition
{
    QString label;
    QString title;
    QString text;
};

namespace PromptLibrary
{
QVector<PromptDefinition> fixedTextPrompts();
PromptDefinition promptByLabel(const QString &label);
}

#endif // PROMPTLIBRARY_H