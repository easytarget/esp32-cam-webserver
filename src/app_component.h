#ifndef app_component_h
#define app_component_h

#include "json_generator.h"
#include "json_parser.h"

#if __has_include("../myconfig.h")
#include "../myconfig.h"
#else
#include "app_config.h"
#endif

#include "storage.h"

#define TAG_LENGTH 32

/**
 * @brief Abstract root class for the application components.
 * 
 */
class CLAppComponent {
    public:
    // Sketch Info
    
        virtual int start(){return OS_SUCCESS;};
        virtual int loadPrefs(){return OS_SUCCESS;};
        virtual int savePrefs(){return OS_SUCCESS;};
        
        virtual void dumpPrefs();
        virtual int removePrefs();
        
        char * getPrefsFileName(bool forsave = false);

        void setDebugMode(bool val) {debug_mode = val;};
        bool isDebugMode(){return debug_mode;};

        int getLastErr() {return last_err;};

        bool isConfigured() {return configured;};

    protected:
        void setTag(const char *t) {tag = t;};
        void setPrefix(const char *p) {prefix = p;};

        void setErr(int err_code) {last_err = err_code;};

        /// @brief reads the Int value from JSON context by token. 
        /// @param jctx JSON context pointer
        /// @param token JSON field where the value is to be retrieved from
        /// @return value, or 0 if fail
        int readJsonIntVal(jparse_ctx_t *jctx, const char* token);

        int parsePrefs(jparse_ctx_t *jctx);

        int urlDecode(char * decoded, char * source, size_t len); 
        int urlEncode(char * encoded, char * source, size_t len);


    private:
        // prefix for forming preference file name of this class
        const char * tag;   
        const char * prefix;

        bool configured = false;

        bool debug_mode = false;

        // error code of the last error
        int last_err = 0;

        char prefs[TAG_LENGTH] = "prefs.json";
};

#endif