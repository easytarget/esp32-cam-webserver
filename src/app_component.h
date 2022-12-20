#ifndef app_component_h
#define app_component_h

#include "json_generator.h"
#include "json_parser.h"

#include "app_config.h"
#include "storage.h"

/**
 * @brief Abstract root class for the appication components.
 * 
 */
class CLAppComponent {
    public:
    // Sketch Info
    
        int start(){return OS_SUCCESS;};
        int loadPrefs(){return OS_SUCCESS;};
        int savePrefs(){return OS_SUCCESS;};
        
        void dumpPrefs();
        int removePrefs();
        
        char * getPrefsFileName(bool forsave = false);

        void setDebugMode(bool val) {debug_mode = val;};
        bool isDebugMode(){return debug_mode;};

        int getLastErr() {return last_err;};

        bool isConfigured() {return configured;};

    protected:
        void setTag(const char *t) {tag = t;};

        void setErr(int err_code) {last_err = err_code;};

        /// @brief reads the Int value from JSON context by token. 
        /// @param jctx JSON context pointer
        /// @param token JSON field where the value is to be retrieved from
        /// @return value, or 0 if fail
        int readJsonIntVal(jparse_ctx_t *jctx, const char* token);

        int parsePrefs(jparse_ctx_t *jctx);


    private:
        // prefix for forming preference file name of this class
        const char * tag;   
        bool configured = false;

        bool debug_mode = false;

        // error code of the last error
        int last_err = 0;

        char prefs[20] = "prefs.json";
};

#endif