/*
 * -----------------------------------------------------------------------------
 * Proyecto: 3 - Servidor BLE con Servicios y Características
 * Autor: Roger Gallegos (adaptado por IA)
 * Fecha: 08-12-2025
 *
 * Objetivo:
 *    Añadir "cajones" de datos (características) a nuestro servidor BLE,
 *    organizados dentro de "contenedores" (servicios). Esto permite a un
 *    cliente que se conecte leer o escribir información específica.
 *
 * Componentes Clave de NimBLE Utilizados:
 *  - ble_gatts_register_svcs(): Registra todos los servicios y características definidos.
 *  - Estructura `ble_gatt_svc_def`: Define un servicio.
 *  - Estructura `ble_gatt_chr_def`: Define una característica.
 *  - BLE_GATT_CHR_F_READ: Flag que indica que una característica se puede leer.
 *
 * Lógica Principal:
 *  1. Se definen UUIDs (Identificadores Únicos Universales) para nuestro servicio
 *     y nuestra característica personalizados.
 *  2. Se crea una función `gatt_svr_chr_access` que el stack llamará cada vez que
 *     un cliente intente leer nuestra característica.
 *  3. Se define un array de `ble_gatt_chr_def` para las características.
 *  4. Se define un array de `ble_gatt_svc_def` que agrupa las características en servicios.
 *  5. En `ble_app_on_sync`, después de iniciar el advertising, se registran los servicios
 *     con `ble_gatts_register_svcs`.
 * -----------------------------------------------------------------------------
*/

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "host/ble_hs.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"

static const char *TAG = "GATT_SVR";

// --- Declaración de UUIDs ---
// Un UUID es un número de 128 bits para identificar servicios y características de forma única.
// Para este ejemplo, usamos un UUID base y lo modificamos para cada servicio/característica.
// Puedes generar tus propios UUIDs en https://www.uuidgenerator.net/
// UUID del servicio: 0000-4A01-4593-9448-1886-DE845028
static const ble_uuid128_t gatt_svr_svc_uuid = BLE_UUID128_INIT(
    0x28, 0x50, 0x84, 0xde, 0x86, 0x18, 0x48, 0x94,
    0x93, 0x45, 0x01, 0x4a, 0x01, 0x00, 0x00, 0x00
);

// UUID de la característica: 0000-4A01-4593-9448-1886-DE845028 con identificador 0x0001
static const ble_uuid128_t gatt_svr_chr_uuid = BLE_UUID128_INIT(
    0x28, 0x50, 0x84, 0xde, 0x86, 0x18, 0x48, 0x94,
    0x93, 0x45, 0x01, 0x4a, 0x01, 0x01, 0x00, 0x00
);

// --- Declaración de Callbacks y Arrays de Definición ---
static int gatt_svr_chr_access(uint16_t conn_handle, uint16_t attr_handle,
                               struct ble_gatt_access_ctxt *ctxt, void *arg);

// Array de características para nuestro servicio.
static const struct ble_gatt_chr_def gatt_svr_chrs[] = {
    {
        // Característica: "Estado del Dispositivo"
        .uuid = &gatt_svr_chr_uuid.u,
        .access_cb = gatt_svr_chr_access,
        .flags = BLE_GATT_CHR_F_READ, // Se puede leer.
    },
    {
        0, // Termina la lista de características.
    }
};

// Array de servicios.
static const struct ble_gatt_svc_def gatt_svr_svcs[] = {
    {
        // Servicio: "Servicio de Información del Dispositivo"
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &gatt_svr_svc_uuid.u,
        .characteristics = gatt_svr_chrs, // Apunta al array de características.
    },
    {
        0, // Termina la lista de servicios.
    }
};

// --- Implementación de Funciones ---

/**
 * @brief Callback de acceso a características GATT.
 * 
 * Esta función es llamada por el stack NimBLE cuando un cliente
 * intenta realizar una operación (leer, escribir) en una característica
 * que tiene este callback asignado.
 */
static int gatt_svr_chr_access(uint16_t conn_handle, uint16_t attr_handle,
                               struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    // --- Código Crítico: Manejo de la lectura de la característica ---
    const char *data = "Hola Mundo BLE!";
    int rc;

    // Verificamos si la operación es una lectura.
    if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
        // Copiamos nuestros datos al buffer de respuesta del stack.
        rc = os_mbuf_append(ctxt->om, data, strlen(data));
        return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
    }

    // Si no es una lectura, no soportamos la operación.
    return BLE_ATT_ERR_REQ_NOT_SUPPORTED;
}

void start_advertising(void)
{
    struct ble_gap_adv_params adv_params;
    struct ble_hs_adv_fields fields;
    int rc;

    memset(&fields, 0, sizeof(fields));
    fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;
    fields.tx_pwr_lvl_is_present = 1;
    fields.tx_pwr_lvl = BLE_HS_ADV_TX_PWR_LVL_AUTO;

    const char *name = ble_svc_gap_device_name();
    fields.name = (uint8_t *)name;
    fields.name_len = strlen(name);
    fields.name_is_complete = 1;

    rc = ble_gap_adv_set_fields(&fields);
    if (rc != 0) {
        ESP_LOGE(TAG, "Error estableciendo los campos del anuncio; rc=%d", rc);
        return;
    }

    memset(&adv_params, 0, sizeof(adv_params));
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;

    rc = ble_gap_adv_start(BLE_OWN_ADDR_PUBLIC, NULL, BLE_HS_FOREVER, &adv_params, NULL, NULL);
    if (rc != 0) {
        ESP_LOGE(TAG, "Error iniciando el advertising; rc=%d", rc);
        return;
    }
    ESP_LOGI(TAG, "Advertising iniciado con el nombre: %s", name);
}

void ble_app_on_sync(void)
{
    int rc;
    ESP_LOGI(TAG, "BLE Host sincronizado.");

    // Deshabilitar servicios por defecto para evitar interferencia
    ble_svc_gap_init();
    ble_svc_gatt_init();

    // Forzar el registro de nuestro servicio personalizado
    rc = ble_gatts_count_cfg(gatt_svr_svcs);
    if (rc != 0) {
        ESP_LOGE(TAG, "Error contando servicios GATT: %d", rc);
        return;
    }

    rc = ble_gatts_add_svcs(gatt_svr_svcs);
    if (rc != 0) {
        ESP_LOGE(TAG, "Error agregando servicios GATT: %d", rc);
        return;
    }

    // Iniciar el servicio GATT explícitamente
    rc = ble_gatts_start();
    if (rc != 0) {
        ESP_LOGE(TAG, "Error iniciando GATT server: %d", rc);
        return;
    }

    ESP_LOGI(TAG, "Servicios GATT registrados y activados exitosamente.");
    ESP_LOGI(TAG, "Servicio personalizado UUID: 0000-4A01-4593-9448-1886-DE845028");

    start_advertising();
}

void ble_host_task(void *param)
{
    ESP_LOGI(TAG, "Tarea del Host BLE iniciada.");
    nimble_port_run();
    nimble_port_freertos_deinit();
}

void app_main(void)
{
    int rc;

    rc = nvs_flash_init();
    if (rc == ESP_ERR_NVS_NO_FREE_PAGES || rc == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        rc = nvs_flash_init();
    }
    ESP_ERROR_CHECK(rc);

    nimble_port_init();

    ble_hs_cfg.sync_cb = ble_app_on_sync;

    const char *device_name = "ESP32-GATT-Svr"; // Cambiamos el nombre para diferenciarlo
    rc = ble_svc_gap_device_name_set(device_name);
    assert(rc == 0);

    nimble_port_freertos_init(ble_host_task);

    ESP_LOGI(TAG, "app_main finalizado.");
}
