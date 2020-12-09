#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_attr.h"
#include "esp_sleep.h"
#include "nvs_flash.h"
#include "protocol_examples_common.h"
#include "esp_sntp.h"
#include <stdio.h>
#include <string.h>

//Led Amarelo
#define LED_Y 2

static const char *TAG = "Obtem Horario";

RTC_DATA_ATTR static int boot_count = 0;

QueueHandle_t filaFuncao;

static void obtain_time(void);
static void initialize_sntp(void);

static void obtain_time(void){

    initialize_sntp();

    //Seta o horario e sincroniza com o esp
    time_t now = 0;
    struct tm timeinfo = { 0 };
    int retry = 0;
    const int retry_count = 10;
    while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET && ++retry < retry_count) {
        ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
    time(&now);
    localtime_r(&now, &timeinfo);

}

static void initialize_sntp(void){

    ESP_LOGI(TAG, "Iniciando SNTP");
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_init();

}

//função acende led
void task_led(void *pvParamters){

	bool estado = 0;
	char strftime_buf[64];

    while(1){


    	xQueueReceive(filaFuncao,&strftime_buf,pdMS_TO_TICKS(2000));
	    ESP_LOGI(TAG, "Horario Led: %s", strftime_buf);

	    estado = !estado;
    	if (strftime_buf[20] == '2' && strftime_buf[21] == '0' && strftime_buf[22] == '2' && strftime_buf[23] == '0') {
    		ESP_LOGI(TAG, "entro!");
	       	gpio_set_level( LED_Y, estado ); 
	    }

	    vTaskDelay(2000/portTICK_PERIOD_MS);

    }
}

//função acende led
void task_hora(void *pvParamters){

	++boot_count;
    ESP_LOGI(TAG, "Boot count: %d", boot_count);

    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    char strftime_buf[64];

    while(1){

	    //Verifica se o tempo foi sincronizado verificando se o ano obtido esta acima de um pré definido
	    if (timeinfo.tm_year < (2019 - 1900)) {
	        obtain_time();
	        //Atualiza o horario atual
	        time(&now);
	    }

	    //Obtem Horario de Brasilia
	    setenv("TZ", "BRT3", 1);
	    tzset();
	    localtime_r(&now, &timeinfo);
	    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
	    ESP_LOGI(TAG, "Horario de Brasilia: %s", strftime_buf);

	    xQueueSend(filaFuncao,&strftime_buf,pdMS_TO_TICKS(0));

	    //Obtem horario de Brasilia com horario de verão
	    setenv("TZ", "BRT3BRST,M11.1.3,M2.3.3", 1);
	    tzset();
	    localtime_r(&now, &timeinfo);
	    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
	    ESP_LOGI(TAG, "Horario de verão fuso de Brasilia: %s", strftime_buf);

	    vTaskDelay(3000/portTICK_PERIOD_MS);

    }
}



void app_main(void){

	gpio_pad_select_gpio( LED_Y ); 	
	gpio_set_direction( LED_Y, GPIO_MODE_OUTPUT );

	ESP_ERROR_CHECK( nvs_flash_init() );
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK( esp_event_loop_create_default() );
    ESP_ERROR_CHECK(example_connect());

    filaFuncao = xQueueCreate(5,sizeof(char[64]));
    ESP_LOGI(TAG, "Inicio...");
    xTaskCreate(task_hora, "task_hora",2048,NULL,5,NULL);
    xTaskCreate(task_led, "task_led",2048,NULL,1,NULL);

}


