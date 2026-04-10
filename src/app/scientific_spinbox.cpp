#include "scientific_spinbox.hpp"


QScientificSpinbox::QScientificSpinbox(QWidget *parent) : QDoubleSpinBox(parent)
{
    this->setDecimals(6);
    this->setMinimum(1e-6);
    this->setMaximum(1e20);
    this->setStepType(StepType::AdaptiveDecimalStepType);
}


QString QScientificSpinbox::textFromValue(double value) const
{
    if (!this->_useScientificNotation || (value > 0.001 && value < 1000 || value == 0)) {
        return QDoubleSpinBox::textFromValue(value);
    } else {
        return this->locale().toString(value, 'E', 6);
    }
}
