/*
 *
 *  BlueZ - Bluetooth protocol stack for Linux
 *
 *  Copyright (C) 2012  Intel Corporation. All rights reserved.
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>

#include <glib.h>

#include "gdbus/gdbus.h"
#include "agent.h"

#define AGENT_PATH "/org/bluez/agent"
#define AGENT_INTERFACE "org.bluez.Agent1"

#include "Log.h"

static gboolean agent_registered = FALSE;
static const char *agent_capability = NULL;
static DBusMessage *pending_message = NULL;

static void agent_release_prompt(void)
{
	if (!pending_message)
		return;

	//bt_shell_release_prompt("");
}

dbus_bool_t agent_completion(void)
{
	if (!pending_message)
		return FALSE;

	return TRUE;
}

static void pincode_response(const char *input, void *user_data)
{
	DBusConnection *conn = user_data;

	g_dbus_send_reply(conn, pending_message, DBUS_TYPE_STRING, &input,
							DBUS_TYPE_INVALID);
}

static void passkey_response(const char *input, void *user_data)
{
	DBusConnection *conn = user_data;
	dbus_uint32_t passkey;

	if (sscanf(input, "%u", &passkey) == 1)
		g_dbus_send_reply(conn, pending_message, DBUS_TYPE_UINT32,
						&passkey, DBUS_TYPE_INVALID);
	else if (!strcmp(input, "no"))
		g_dbus_send_error(conn, pending_message,
					"org.bluez.Error.Rejected", NULL);
	else
		g_dbus_send_error(conn, pending_message,
					"org.bluez.Error.Canceled", NULL);
}

static void confirm_response(const char *input, void *user_data)
{
	DBusConnection *conn = user_data;

	if (!strcmp(input, "yes"))
		g_dbus_send_reply(conn, pending_message, DBUS_TYPE_INVALID);
	else if (!strcmp(input, "no"))
		g_dbus_send_error(conn, pending_message,
					"org.bluez.Error.Rejected", NULL);
	else
		g_dbus_send_error(conn, pending_message,
					"org.bluez.Error.Canceled", NULL);
}

static void agent_release(DBusConnection *conn)
{
	agent_registered = FALSE;
	agent_capability = NULL;

	if (pending_message) {
		dbus_message_unref(pending_message);
		pending_message = NULL;
	}

	agent_release_prompt();

	g_dbus_unregister_interface(conn, AGENT_PATH, AGENT_INTERFACE);
}

static DBusMessage *release_agent(DBusConnection *conn,
					DBusMessage *msg, void *user_data)
{
    UNUSED(user_data);

    LOGD("Agent released");

	agent_release(conn);

	return dbus_message_new_method_return(msg);
}

static DBusMessage *request_pincode(DBusConnection *conn,
					DBusMessage *msg, void *user_data)
{
    UNUSED(conn);
    UNUSED(user_data);

	const char *device;

    LOGD("Request PIN code");

	dbus_message_get_args(msg, NULL, DBUS_TYPE_OBJECT_PATH, &device,
							DBUS_TYPE_INVALID);

    // TODO
	//bt_shell_prompt_input("agent", "Enter PIN code:", pincode_response, conn);
    UNUSED(pincode_response);

	pending_message = dbus_message_ref(msg);

	return NULL;
}

static DBusMessage *display_pincode(DBusConnection *conn,
					DBusMessage *msg, void *user_data)
{
    UNUSED(conn);
    UNUSED(user_data);

	const char *device;
	const char *pincode;

	dbus_message_get_args(msg, NULL, DBUS_TYPE_OBJECT_PATH, &device,
				DBUS_TYPE_STRING, &pincode, DBUS_TYPE_INVALID);

    LOGI("PIN code: %s", pincode);

	return dbus_message_new_method_return(msg);
}

static DBusMessage *request_passkey(DBusConnection *conn,
					DBusMessage *msg, void *user_data)
{
    UNUSED(conn);
    UNUSED(user_data);

	const char *device;

    LOGI("Request passkey");

	dbus_message_get_args(msg, NULL, DBUS_TYPE_OBJECT_PATH, &device,
							DBUS_TYPE_INVALID);

    // TODO
	//bt_shell_prompt_input("agent", "Enter passkey (number in 0-999999):", passkey_response, conn);
    UNUSED(passkey_response);

	pending_message = dbus_message_ref(msg);

	return NULL;
}

static DBusMessage *display_passkey(DBusConnection *conn,
					DBusMessage *msg, void *user_data)
{
    UNUSED(conn);
    UNUSED(user_data);

	const char *device;
	dbus_uint32_t passkey;
	dbus_uint16_t entered;
	char passkey_full[7];

	dbus_message_get_args(msg, NULL, DBUS_TYPE_OBJECT_PATH, &device,
			DBUS_TYPE_UINT32, &passkey, DBUS_TYPE_UINT16, &entered,
							DBUS_TYPE_INVALID);

	snprintf(passkey_full, sizeof(passkey_full), "%.6u", passkey);
	passkey_full[6] = '\0';

	if (entered > strlen(passkey_full))
		entered = strlen(passkey_full);

    // TODO
	//bt_shell_printf(AGENT_PROMPT "Passkey: "
	//		COLOR_BOLDGRAY "%.*s" COLOR_BOLDWHITE "%s\n" COLOR_OFF,
	//			entered, passkey_full, passkey_full + entered);

	return dbus_message_new_method_return(msg);
}

static DBusMessage *request_confirmation(DBusConnection *conn,
					DBusMessage *msg, void *user_data)
{
    UNUSED(conn);
    UNUSED(user_data);

	const char *device;
	dbus_uint32_t passkey;
	char *str;

    LOGI("Request confirmation");

	dbus_message_get_args(msg, NULL, DBUS_TYPE_OBJECT_PATH, &device,
				DBUS_TYPE_UINT32, &passkey, DBUS_TYPE_INVALID);

	str = g_strdup_printf("Confirm passkey %06u (yes/no):", passkey);

    // TODO
	// bt_shell_prompt_input("agent", str, confirm_response, conn);
    UNUSED(confirm_response);

	g_free(str);

	pending_message = dbus_message_ref(msg);

	return NULL;
}

static DBusMessage *request_authorization(DBusConnection *conn,
					DBusMessage *msg, void *user_data)
{
    UNUSED(conn);
    UNUSED(user_data);

	const char *device;

    LOGI("Request authorization");

	dbus_message_get_args(msg, NULL, DBUS_TYPE_OBJECT_PATH, &device,
							DBUS_TYPE_INVALID);

    // TODO
	//bt_shell_prompt_input("agent", "Accept pairing (yes/no):", confirm_response, conn);
    UNUSED(confirm_response);

	pending_message = dbus_message_ref(msg);

	return NULL;
}

static DBusMessage *authorize_service(DBusConnection *conn,
					DBusMessage *msg, void *user_data)
{
    UNUSED(conn);
    UNUSED(user_data);

	const char *device, *uuid;
	char *str;

	LOGI("Authorize service");

	dbus_message_get_args(msg, NULL, DBUS_TYPE_OBJECT_PATH, &device,
				DBUS_TYPE_STRING, &uuid, DBUS_TYPE_INVALID);

	str = g_strdup_printf("Authorize service %s (yes/no):", uuid);

    // TODO
	// bt_shell_prompt_input("agent", str, confirm_response, conn);
    UNUSED(confirm_response);

	g_free(str);

	pending_message = dbus_message_ref(msg);

	return NULL;
}

static DBusMessage *cancel_request(DBusConnection *conn,
					DBusMessage *msg, void *user_data)
{
    UNUSED(conn);
    UNUSED(user_data);

	LOGI("Request canceled");

	agent_release_prompt();
	dbus_message_unref(pending_message);
	pending_message = NULL;

	return dbus_message_new_method_return(msg);
}

static const GDBusMethodTable methods[] = {
	{ GDBUS_METHOD("Release", NULL, NULL, release_agent) },
	{ GDBUS_ASYNC_METHOD("RequestPinCode",
			GDBUS_ARGS({ "device", "o" }),
			GDBUS_ARGS({ "pincode", "s" }), request_pincode) },
	{ GDBUS_METHOD("DisplayPinCode",
			GDBUS_ARGS({ "device", "o" }, { "pincode", "s" }),
			NULL, display_pincode) },
	{ GDBUS_ASYNC_METHOD("RequestPasskey",
			GDBUS_ARGS({ "device", "o" }),
			GDBUS_ARGS({ "passkey", "u" }), request_passkey) },
	{ GDBUS_METHOD("DisplayPasskey",
			GDBUS_ARGS({ "device", "o" }, { "passkey", "u" },
							{ "entered", "q" }),
			NULL, display_passkey) },
	{ GDBUS_ASYNC_METHOD("RequestConfirmation",
			GDBUS_ARGS({ "device", "o" }, { "passkey", "u" }),
			NULL, request_confirmation) },
	{ GDBUS_ASYNC_METHOD("RequestAuthorization",
			GDBUS_ARGS({ "device", "o" }),
			NULL, request_authorization) },
	{ GDBUS_ASYNC_METHOD("AuthorizeService",
			GDBUS_ARGS({ "device", "o" }, { "uuid", "s" }),
			NULL,  authorize_service) },
	{ GDBUS_METHOD("Cancel", NULL, NULL, cancel_request) },
	{ }
};

static void register_agent_setup(DBusMessageIter *iter, void *user_data)
{
    UNUSED(user_data);

	const char *path = AGENT_PATH;
	const char *capability = agent_capability;

	dbus_message_iter_append_basic(iter, DBUS_TYPE_OBJECT_PATH, &path);
	dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &capability);
}

static void register_agent_reply(DBusMessage *message, void *user_data)
{
	DBusConnection *conn = user_data;
	DBusError error;

	dbus_error_init(&error);

	if (dbus_set_error_from_message(&error, message) == FALSE) {
		agent_registered = TRUE;
		LOGI("Agent registered");
	} else {
		LOGE("Failed to register agent: %s", error.name);
		dbus_error_free(&error);

		if (g_dbus_unregister_interface(conn, AGENT_PATH,
						AGENT_INTERFACE) == FALSE)
			LOGE("Failed to unregister agent object");
	}
}

void agent_register(DBusConnection *conn, GDBusProxy *manager,
						const char *capability)

{
	if (agent_registered == TRUE) {
		LOGW("Agent is already registered");
		return;
	}

	agent_capability = capability;

	if (g_dbus_register_interface(conn, AGENT_PATH,
					AGENT_INTERFACE, methods,
					NULL, NULL, NULL, NULL) == FALSE) {
		LOGE("Failed to register agent object");
		return;
	}

	if (g_dbus_proxy_method_call(manager, "RegisterAgent",
						register_agent_setup,
						register_agent_reply,
						conn, NULL) == FALSE) {
		LOGE("Failed to call register agent method");
		return;
	}

	agent_capability = NULL;
}

static void unregister_agent_setup(DBusMessageIter *iter, void *user_data)
{
    UNUSED(user_data);

	const char *path = AGENT_PATH;

	dbus_message_iter_append_basic(iter, DBUS_TYPE_OBJECT_PATH, &path);
}

static void unregister_agent_reply(DBusMessage *message, void *user_data)
{
	DBusConnection *conn = user_data;
	DBusError error;

	dbus_error_init(&error);

	if (dbus_set_error_from_message(&error, message) == FALSE) {
		LOGI("Agent unregistered");
		agent_release(conn);
	} else {
		LOGE("Failed to unregister agent: %s", error.name);
		dbus_error_free(&error);
	}
}

void agent_unregister(DBusConnection *conn, GDBusProxy *manager)
{
	if (agent_registered == FALSE) {
		LOGW("No agent is registered");
		return;
	}

	if (!manager) {
		LOGI("Agent unregistered");
		agent_release(conn);
		return;
	}

	if (g_dbus_proxy_method_call(manager, "UnregisterAgent",
						unregister_agent_setup,
						unregister_agent_reply,
						conn, NULL) == FALSE) {
		LOGE("Failed to call unregister agent method");
		return;
	}
}

static void request_default_setup(DBusMessageIter *iter, void *user_data)
{
    UNUSED(user_data);

	const char *path = AGENT_PATH;

	dbus_message_iter_append_basic(iter, DBUS_TYPE_OBJECT_PATH, &path);
}

static void request_default_reply(DBusMessage *message, void *user_data)
{
    UNUSED(user_data);

	DBusError error;

	dbus_error_init(&error);

	if (dbus_set_error_from_message(&error, message) == TRUE) {
		LOGE("Failed to request default agent: %s",
							error.name);
		dbus_error_free(&error);
        // TODO
        return;
		//return bt_shell_noninteractive_quit(EXIT_FAILURE);
	}

	LOGI("Default agent request successful");

    // TODO
	//return bt_shell_noninteractive_quit(EXIT_SUCCESS);
}

void agent_default(DBusConnection *conn, GDBusProxy *manager)
{
    UNUSED(conn);

	if (agent_registered == FALSE) {
		LOGW("No agent is registered");
        // TODO
		//return bt_shell_noninteractive_quit(EXIT_FAILURE);
        return;
	}

	if (g_dbus_proxy_method_call(manager, "RequestDefaultAgent",
						request_default_setup,
						request_default_reply,
						NULL, NULL) == FALSE) {
		LOGE("Failed to call RequestDefaultAgent method");
        // TODO
		//return bt_shell_noninteractive_quit(EXIT_FAILURE);
        return;
	}
}
