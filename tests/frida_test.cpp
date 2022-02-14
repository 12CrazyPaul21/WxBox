#include <test_common.h>
#include <frida-gum.h>

std::string show_message(int id)
{
    spdlog::info("before hook show message id : {}", id);
    return "before hook";
}

std::string attached_show_message(int id)
{
    spdlog::info("after hook show message id : {}", id);
    return "after hook";
}

G_BEGIN_DECLS

//
// define hook listener
//

typedef enum _WxBoxHookId
{
    SHOW_MESSAGE,
    MESSAGE_BEEP
} WxBoxHookId;

typedef struct _WxBoxHookListener
{
    GObject parent;
} WxBoxHookListener;

static void wxbox_hook_listener_iface_init(gpointer g_iface, gpointer iface_data);

#define WXBOX_TYPE_HOOK_LISTENER (wxbox_hook_listener_get_type())
G_DECLARE_FINAL_TYPE(WxBoxHookListener, wxbox_hook_listener, WXBOX, HOOK_LISTENER, GObject)
G_DEFINE_TYPE_EXTENDED(WxBoxHookListener, wxbox_hook_listener, G_TYPE_OBJECT, 0, G_IMPLEMENT_INTERFACE(GUM_TYPE_INVOCATION_LISTENER, wxbox_hook_listener_iface_init))

static void wxbox_hook_listener_on_enter(GumInvocationListener* listener, GumInvocationContext* ic)
{
    spdlog::info("wxbox_hook_listener_on_enter");

    //WxBoxHookListener* self   = WXBOX_HOOK_LISTENER(listener);
    WxBoxHookId hookId = (WxBoxHookId)GUM_IC_GET_FUNC_DATA(ic, int);

    switch (hookId) {
        case SHOW_MESSAGE: {
            auto id = (int)gum_invocation_context_get_nth_argument(ic, 1);
            attached_show_message(id);
            gum_invocation_context_replace_nth_argument(ic, 1, GINT_TO_POINTER(id + 1));
            break;
        }
        case MESSAGE_BEEP: {
            auto arg = (int)gum_invocation_context_get_nth_argument(ic, 0);
            spdlog::info("message beep arg: {}", arg);
            break;
        }
    }
}

static void wxbox_hook_listener_on_leave(GumInvocationListener* listener, GumInvocationContext* ic)
{
    spdlog::info("wxbox_hook_listener_on_leave");
}

static void wxbox_hook_listener_class_init(WxBoxHookListenerClass* kclass)
{
    spdlog::info("wxbox_hook_listener_class_init");
}

static void wxbox_hook_listener_init(WxBoxHookListener* self)
{
    spdlog::info("wxbox_hook_listener_init");
}

static void wxbox_hook_listener_iface_init(gpointer g_iface, gpointer iface_data)
{
    GumInvocationListenerInterface* iface = (GumInvocationListenerInterface*)g_iface;
    iface->on_enter                       = wxbox_hook_listener_on_enter;
    iface->on_leave                       = wxbox_hook_listener_on_leave;
}

G_END_DECLS

TEST(frida_gum, hook)
{
    GumInterceptor*        interceptor;
    GumInvocationListener* listener;

    gum_init_embedded();

    interceptor = gum_interceptor_obtain();
    listener    = (GumInvocationListener*)g_object_new(WXBOX_TYPE_HOOK_LISTENER, NULL);

    gum_interceptor_begin_transaction(interceptor);
    gum_interceptor_attach(interceptor,
                           GSIZE_TO_POINTER(show_message),
                           listener,
                           GSIZE_TO_POINTER(SHOW_MESSAGE));
    //gum_interceptor_attach(interceptor,
    //                       GSIZE_TO_POINTER(gum_module_find_export_by_name("user32.dll", "MessageBeep")),
    //                       listener,
    //                       GSIZE_TO_POINTER(MESSAGE_BEEP));
    gum_interceptor_end_transaction(interceptor);

    show_message(1);
    //MessageBeep(MB_ICONINFORMATION);
    gum_interceptor_detach(interceptor, listener);
    show_message(1);

    g_object_unref(listener);
    g_object_unref(interceptor);
    gum_deinit_embedded();
}