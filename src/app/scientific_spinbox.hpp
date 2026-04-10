#pragma once
#include <QtWidgets/QDoubleSpinBox>
#include <settings.hpp>

class QScientificSpinbox : public QDoubleSpinBox
{
    Q_OBJECT
public:
    explicit QScientificSpinbox(QWidget *parent = nullptr);

    double valueFromText(const QString & text) const
    {
        return this->locale().toDouble(text);
    }

    QString textFromValue(double value) const;

    QValidator::State validate(QString &text, int&) const
    {

        bool ok;
        this->locale().toDouble(text, &ok);
        return ok ? QValidator::Acceptable : QValidator::Invalid;
    }

    inline void setUseScientificNotation(bool enable)
    {
        this->_useScientificNotation = enable;
        this->setValue(this->value());
    }

private:
    bool _useScientificNotation = true;
};
