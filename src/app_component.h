#ifndef app_component_h
#define app_component_h

#include <json_generator.h>
#include <json_parser.h>

#include "app_config.h"
#include "storage.h"

class CLAppComponent {
    public:
    // Sketch Info
    
        int start(){return OS_SUCCESS;};
        int loadPrefs(){return OS_SUCCESS;};
        int savePrefs(){return OS_SUCCESS;};
        
        void dumpPrefs();
        void removePrefs();
        
        char * getPrefsFileName(bool forsave = false);

        void setDebugMode(bool val) {debug_mode = val;};
        bool isDebugMode(){return debug_mode;};

    protected:
        void setTag(const char *t) {tag = t;};

        /// @brief reads the Int value from JSON context by token. 
        /// @param jctx_ptr JSON context pointer
        /// @param token 
        /// @return value, or 0 if fail
        int readJsonIntVal(jparse_ctx_t *jctx, char* token);

        int parsePrefs(jparse_ctx_t *jctx);


    private:
        const char * tag;

        bool debug_mode = false;

        char prefs[20] = "prefs.json";
};

#endif