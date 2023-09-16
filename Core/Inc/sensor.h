#ifndef F746_DISCO_HEART_SENSOR_H
#define F746_DISCO_HEART_SENSOR_H

#ifdef __cplusplus
extern "C" {
#endif


#define SAMPLES_PER_SEC 100

typedef struct {
    uint16_t ir;
    uint16_t red;
} SensorSample_t;

bool sendData(SensorSample_t const *sample);

bool receiveData(SensorSample_t *sample, unsigned int ticksToWait);

#ifdef __cplusplus
};
#endif

#endif //F746_DISCO_HEART_SENSOR_H
