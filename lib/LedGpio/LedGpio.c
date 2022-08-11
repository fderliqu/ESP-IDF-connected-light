#include "esp_log.h"
#include "esp_event.h"
#include"driver/gpio.h"
#include"pca9685.h"
#include"string.h"

#include"LedGpio.h"

#define ADDR PCA9685_ADDR_BASE
#define GPIO_SCL GPIO_NUM_19
#define GPIO_SDA GPIO_NUM_18
#define BASE_FREQ 500
#define RED_CHANNEL PCA9685_CHANNEL_0
#define GREEN_CHANNEL PCA9685_CHANNEL_4
#define BLUE_CHANNEL PCA9685_CHANNEL_8

static const char *TAG = "[PCA9685]";

i2c_dev_t dev;
bool is_sleeping;
uint16_t val1 = 0,val2=0,val3=0;

void init_pwm(){

    ESP_ERROR_CHECK(i2cdev_init());

    memset(&dev,0,sizeof(i2c_dev_t));
    ESP_ERROR_CHECK(pca9685_init_desc(&dev,ADDR,0,GPIO_SDA,GPIO_SCL));
    ESP_ERROR_CHECK(pca9685_init(&dev));
    ESP_ERROR_CHECK(pca9685_restart(&dev));
    
    uint16_t freq;

    ESP_ERROR_CHECK(pca9685_set_pwm_frequency(&dev, BASE_FREQ));
    ESP_ERROR_CHECK(pca9685_get_pwm_frequency(&dev, &freq));

    ESP_LOGI(TAG, "Freq %dHz, real %d", BASE_FREQ, freq);
}

esp_err_t OnLed(){
    esp_err_t status;
    ESP_ERROR_CHECK(pca9685_is_sleeping(&dev,&is_sleeping));
    if(is_sleeping)status = pca9685_sleep(&dev,false);
    else status = ESP_OK;
    return status;
}

esp_err_t OffLed(){
    esp_err_t status;
    ESP_ERROR_CHECK(pca9685_is_sleeping(&dev,&is_sleeping));
    if(!is_sleeping)status = pca9685_sleep(&dev,true);
    else status = ESP_OK;
    return status;
}

void SetColorValues(char * hex, uint16_t a){
    uint16_t red,blue,green;
    //float brightness = a/255;
    float brightness = a*0 + 1;
    int status = sscanf(hex, "#%02hx%02hx%02hx", &red, &green, &blue);
    if(status == 3){
        ESP_LOGD(TAG,"Color : %d,%d,%d",red,green,blue);
        red = (15+(red<<4))*brightness;
        green = (15+(green<<4))*brightness;
        blue = (15+(blue<<4))*brightness;
        if(pca9685_set_pwm_value(&dev,RED_CHANNEL,red) != ESP_OK)ESP_LOGE(TAG, "Could not set PWM value to ch0");
        if(pca9685_set_pwm_value(&dev,GREEN_CHANNEL,green) != ESP_OK)ESP_LOGE(TAG, "Could not set PWM value to ch4");
        if(pca9685_set_pwm_value(&dev,BLUE_CHANNEL,blue) != ESP_OK)ESP_LOGE(TAG, "Could not set PWM value to ch8");
    }
}

void SetWhiteValues(){
    if(pca9685_set_pwm_value(&dev,RED_CHANNEL,PCA9685_MAX_PWM_VALUE-1) != ESP_OK)ESP_LOGE(TAG, "Could not set PWM value to ch0");
    if(pca9685_set_pwm_value(&dev,GREEN_CHANNEL,PCA9685_MAX_PWM_VALUE-1) != ESP_OK)ESP_LOGE(TAG, "Could not set PWM value to ch4");
    if(pca9685_set_pwm_value(&dev,BLUE_CHANNEL,PCA9685_MAX_PWM_VALUE-1) != ESP_OK)ESP_LOGE(TAG, "Could not set PWM value to ch8");
}

void ServoTest(){
    ESP_LOGI(TAG,"test servo %d",val1/208);
    if(pca9685_set_pwm_value(&dev,RED_CHANNEL,val1) != ESP_OK)ESP_LOGE(TAG, "Could not set PWM value to ch0");
    val1+=10;
    if(val1>4098)val1=0;
    //if(pca9685_set_pwm_value(&dev,GREEN_CHANNEL,1500) != ESP_OK)ESP_LOGE(TAG, "Could not set PWM value to ch4");
    //if(pca9685_set_pwm_value(&dev,BLUE_CHANNEL,3000) != ESP_OK)ESP_LOGE(TAG, "Could not set PWM value to ch8");
}

void scan_i2c(){
    i2c_config_t conf;
    ESP_ERROR_CHECK(i2cdev_init());
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = 18;
    conf.scl_io_num = 19;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    ESP_ERROR_CHECK(i2c_param_config(I2C_NUM_0, &conf));
    printf("- i2c controller configured\r\n");
// install the driver
    ESP_ERROR_CHECK(i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0));
    printf("- i2c driver installed\r\n\r\n");
    printf("scanning the bus...\r\n\r\n"); 
    int devices_found = 0;
    for(int address = 1; address < 127; address++) {
	// create and execute the command link
	    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	    i2c_master_start(cmd);
	    i2c_master_write_byte(cmd, (address << 1) | I2C_MASTER_WRITE, true);
	    i2c_master_stop(cmd);
	    if(i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000 / portTICK_RATE_MS) == ESP_OK) {
		    printf("-> found device with address 0x%02x\r\n", address);
		    devices_found++;
	    }
	i2c_cmd_link_delete(cmd);
    }
    if(devices_found == 0) printf("\r\n-> no devices found\r\n");
    printf("\r\n...scan completed!\r\n");   
}

