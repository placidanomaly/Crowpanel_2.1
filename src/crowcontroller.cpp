#include "crowpanel_2_1.h"

CrowController::CrowController() {
    allControllers.push_back(this);
}

CrowController::~CrowController() {
    std::erase_if(CrowController::allControllers, [this](const CrowController * p) { 
        return p == this;
    });
}

void CrowController::begin(Crowpanel &panel) {

    this->panel = &panel;
    if(panel.wokeUpFromDeepSleep()) restoreStateAfterDeepSleep();

    resetSleepTimeout();
}

void CrowController::activate() {

    if(panel == NULL) {
        debug << "ERROR: CrowController::activate() called before CrowController::begin(Crowpanel &)" << endl;
        return;
    }

    active = true;

    panel->registerClickActionCallback([](void *context) {
        CrowController *cc = static_cast<CrowController *>(context);
        cc->resetSleepTimeout();
        cc->click();
    }, this);

    panel->registerDoubleClickActionCallback([](void *context) {
        CrowController *cc = static_cast<CrowController *>(context);
        cc->resetSleepTimeout();
        cc->doubleClick();
    }, this);

    panel->registerLongPressActionCallback([](void *context) {
        CrowController *cc = static_cast<CrowController *>(context);
        cc->resetSleepTimeout();
        cc->longPress();
    }, this);

    panel->registerEncoderRotationActionCallback([](int rotation, void *context) {
        CrowController *cc = static_cast<CrowController *>(context);
        cc->resetSleepTimeout();
        cc->encoderRotation(rotation);
    }, this, true);

    resetSleepTimeout();
}

void CrowController::deactivate() {
    active = false;

    panel->registerClickActionCallback(NULL, NULL);
    panel->registerDoubleClickActionCallback(NULL, NULL);
    panel->registerLongPressActionCallback(NULL, NULL);
    panel->registerEncoderRotationActionCallback(NULL, NULL, false);
}

bool CrowController::isActivated() {
    return active;
}

void CrowController::resetSleepTimeout() {
    lastActiveMillis = millis();
}

void CrowController::goToDeepSleep() {
    for(auto c = allControllers.begin(); c != allControllers.end(); ++c) (*c)->saveStateBeforeDeepSleep();
    panel->goToDeepSleep();
}

void CrowController::goToLightSleep() {
    
    // Disable all user inputs
    deactivate();

    panel->goToLightSleep();

    // Ignore the inputs that triggered wake up for a little while so unwanted
    // changes don't get applied
    delay(250);
    
    // We just woke up so reset sleep timeout
    resetSleepTimeout();
    
    // Reactivate user input handling
    activate();
}

void CrowController::setSleepTimeout(unsigned long sleepSeconds) {
    sleepTimeout = sleepSeconds * 1000;
}

void CrowController::setDeepSleepTimeout(unsigned long sleepSeconds) {
    deepSleepTimeout = sleepSeconds * 1000;
}

void CrowController::loop() {
    panel->loop();

    if(sleepTimeout > 0)
        if(millis() - lastActiveMillis > sleepTimeout) goToLightSleep();
    if(deepSleepTimeout > 0)
        if(millis() - lastActiveMillis > deepSleepTimeout) goToDeepSleep();
}
