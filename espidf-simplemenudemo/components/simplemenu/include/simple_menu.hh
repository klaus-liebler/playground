#pragma once

#include <cstdint>
#include <cstdio>
#include <vector>
#include <interfaces.hh>
#define TAG "MENU"
#include <esp_log.h>
#include <symbols.hh>
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"

using namespace display;


namespace menu
{
     inline size_t modulo(const int i, const int n)
    {
        return (i % n + n) % n;
    }
    
    enum class MenuItemResult
    {
        NO_ACTION,
        REDRAW_SECOND_LINE,
        OPEN_NEW_FULLSCREEN,
        CLOSE_MYSELF,
        REDRAW,
    };
    enum class OnOpenFullscreenResult
    {
        OK,
        NOT_OK,                           // for PlaceholderItem
        NOT_OK_AND_CLOSE_PREVIOUS_WINDOW, // for ReturnItem
    };

    class MenuItem;

    template <class T>
    class MenuItemChanged
    {
    public:
        virtual void ValueChanged(const MenuItem *item, T newValue) = 0;
    };
    
    template <class T>
    class MenuItemChangedWithHandle
    {
    public:
        virtual void ValueChanged(const MenuItem *item, void* referenceOrHandle, T newValue) = 0;
    };

    class MenuItem
    {
    protected:
        const char *const name;

    public:
        MenuItem(const char *const name) : name(name) {}
        virtual void RenderCompact(FullLineWriter *lw, int line, bool invert) = 0;
        virtual void RenderFullScreen(FullLineWriter *lw, bool initial, uint8_t shownLines, uint8_t availableLines){};
        virtual OnOpenFullscreenResult OnOpenFullscreen() { return OnOpenFullscreenResult::OK; } // This is the normal case
        virtual void OnCloseFullscreen() { return; }                                             // This is the normal case
        virtual MenuItemResult Up() { return MenuItemResult::CLOSE_MYSELF; }
        virtual MenuItemResult Down() { return MenuItemResult::CLOSE_MYSELF; }
        virtual MenuItemResult Select(MenuItem **toOpen) { return MenuItemResult::CLOSE_MYSELF; }
        virtual MenuItemResult Back() { return MenuItemResult::CLOSE_MYSELF; }
        const char *GetName() const { return name; }

        virtual esp_err_t SaveToNvs(nvs_handle_t& nvs_handle){
            return ESP_OK;
        }

        virtual esp_err_t OpenFromNvs(nvs_handle_t& nvs_handle){
            return ESP_OK;
        }
    };

    class ConfirmationItem : public MenuItem
    {
    private:
        void* referenceOrHandle;
        MenuItemChangedWithHandle<bool> *cb;
        static bool confirmationTmp;

    public:
        ConfirmationItem(const char *const name, void* referenceOrHandle, MenuItemChangedWithHandle<bool> *cb = nullptr) : MenuItem(name), referenceOrHandle(referenceOrHandle), cb(cb) {}
        void RenderCompact(FullLineWriter *lw, int line, bool invert) override
        {
            lw->printfl(line, invert, "%s" G_LABEL_ALT, name);
        }

        MenuItemResult Select(MenuItem **toOpen) override {
            (void)toOpen;
             if (cb)
                cb->ValueChanged(this, this->referenceOrHandle, confirmationTmp);
            return MenuItemResult::CLOSE_MYSELF;
        }

        void RenderFullScreen(FullLineWriter *lw, bool initial, uint8_t shownLines, uint8_t availableLines) override
        {
            if (initial)
            {
                lw->ClearScreenAndResetStartline();
                lw->printfl(0, false, "Are you sure?");
                confirmationTmp=false;
            }
            lw->printfl(1, true, ">[%s]", confirmationTmp?"Yes":"No");
        }
        MenuItemResult Up() override
        {
            confirmationTmp=!confirmationTmp;
            return MenuItemResult::REDRAW;
        }
        MenuItemResult Down() override
        {
            confirmationTmp=!confirmationTmp;
            return MenuItemResult::REDRAW;
        }
    };

    

    class IntegerItem : public MenuItem
    {
    private:
        int32_t *value;
        int32_t value_min;
        int32_t value_max;
        MenuItemChanged<int32_t> *cb;
        static int32_t valueTmp;

    public:
        IntegerItem(const char *const name, int32_t *value, int32_t value_min, int32_t value_max, MenuItemChanged<int32_t> *cb = nullptr) : MenuItem(name), value(value), value_min(value_min), value_max(value_max), cb(cb) {}
        void RenderCompact(FullLineWriter *lw, int line, bool invert) override
        {
            lw->printfl(line, invert, "%s\t\t%d", name, *value);
        }

        esp_err_t SaveToNvs(nvs_handle_t& nvs_handle) override{
            return nvs_set_i32(nvs_handle, name, *value);
        }

        esp_err_t OpenFromNvs(nvs_handle_t& nvs_handle) override{
            return nvs_get_i32(nvs_handle, name, value);
        }

        MenuItemResult Select(MenuItem **toOpen) override {
            *value=valueTmp;
             if (cb)
                cb->ValueChanged(this, *value);
            return MenuItemResult::CLOSE_MYSELF;
        }

        void RenderFullScreen(FullLineWriter *lw, bool initial, uint8_t shownLines, uint8_t availableLines) override
        {
            if (initial)
            {
                valueTmp=*value;
                lw->ClearScreenAndResetStartline();
                lw->printfl(0, false, "Edit Value");
            }
            lw->printfl(1, true, "%d [%d]", valueTmp, *value);
        }
        MenuItemResult Up() override
        {
            valueTmp++;
            if (valueTmp > value_max)
                valueTmp = value_min;
           
            return MenuItemResult::REDRAW;
        }
        MenuItemResult Down() override
        {
            valueTmp--;
            if (valueTmp < value_min)
                valueTmp = value_max;
            return MenuItemResult::REDRAW;
        }
    };

    class BoolItem : public MenuItem
    {
    private:
        bool *value;
        MenuItemChanged<bool> *cb;
        static bool valueTmp;

    public:
        BoolItem(const char *const name, bool *value, MenuItemChanged<bool> *cb = nullptr) : MenuItem(name), value(value), cb(cb) {}
        
        esp_err_t SaveToNvs(nvs_handle_t& nvs_handle) override{
            return nvs_set_i32(nvs_handle, name, *value?1:0);
        }

        esp_err_t OpenFromNvs(nvs_handle_t& nvs_handle) override{
            int32_t foo=0;
            auto ret = nvs_get_i32(nvs_handle, name, &foo);
            *value=foo==1?true:false;
            return ret;
        }
        
        void RenderCompact(FullLineWriter *lw, int line, bool invert) override
        {
            if (*value)
            {
                lw->printfl(line, invert, "%s\t\t" G_CHECKBOX_ON, name);
            }
            else
            {
                lw->printfl(line, invert, "%s\t\t" G_CHECKBOX, name);
            }
        }

        MenuItemResult Select(MenuItem **toOpen) override {
            *value=valueTmp;
             if (cb)
                cb->ValueChanged(this, *value);
            return MenuItemResult::CLOSE_MYSELF;
        }

        void RenderFullScreen(FullLineWriter *lw, bool initial, uint8_t shownLines, uint8_t availableLines) override
        {
            if (initial)
            {
                valueTmp=*value;
                lw->ClearScreenAndResetStartline();
                lw->printfl(0, false, "Edit Value");
            }
            
            lw->printfl(1, true, "%s (%s)", valueTmp?G_CHECKBOX_ON:G_CHECKBOX, valueTmp==*value?"orig":"changed");
            
        }

        MenuItemResult Up() override
        {
            valueTmp = !valueTmp;
            return MenuItemResult::REDRAW;
        }
        MenuItemResult Down() override
        {
            valueTmp = !valueTmp;
            return MenuItemResult::REDRAW;
        }
    };

    template <uint16_t UNITS_PER_INTEGER = 10>
    class FixedPointItem : public MenuItem
    {
    private:
        int32_t value_min, value_max;
        float *value;
        static int valueTmp;
        MenuItemChanged<float> *cb;

    public:
        FixedPointItem(const char *const name, float *value, float value_min, float value_max, MenuItemChanged<float> *cb = nullptr) : MenuItem(name),
                                                                                                                            
                                                                                                                                       value_min(value_min * UNITS_PER_INTEGER),
                                                                                                                                       value_max(value_max * UNITS_PER_INTEGER),
                                                                                                                                       value(value),
                                                                                                                                       cb(cb)
        {
        }


        esp_err_t SaveToNvs(nvs_handle_t& nvs_handle) override{
            return nvs_set_i32(nvs_handle, name, (*value)*(float)UNITS_PER_INTEGER);
        }

        esp_err_t OpenFromNvs(nvs_handle_t& nvs_handle) override{
            int32_t foo=0;
            auto ret = nvs_get_i32(nvs_handle, name, &foo);
            *value=foo/(float)UNITS_PER_INTEGER;
            return ret;
        }

        void RenderCompact(FullLineWriter *lw, int line, bool invert) override
        {
            lw->printfl(line, invert, "%s\t\t%.2f", name, *value);
        }

        void RenderFullScreen(FullLineWriter *lw, bool initial, uint8_t shownLines, uint8_t availableLines) override
        {
            if (initial)
            {
                valueTmp=(*value)*(float)UNITS_PER_INTEGER;
                lw->ClearScreenAndResetStartline();
                lw->printfl(0, false, "Edit Value");
            }
            lw->printfl(1, true, "%.2f [%.2f]", valueTmp/(float)UNITS_PER_INTEGER, *value);
        }

         MenuItemResult Select(MenuItem **toOpen) override {
            *value=valueTmp/(float)UNITS_PER_INTEGER;
             if (cb)
                cb->ValueChanged(this, *value);
            return MenuItemResult::CLOSE_MYSELF;
        }


        MenuItemResult Up() override
        {
            valueTmp++;
            if (valueTmp > value_max)
                valueTmp = value_min;
            return MenuItemResult::REDRAW;
        }
        MenuItemResult Down() override
        {
            valueTmp--;
            if (valueTmp < value_min)
                valueTmp = value_max;
            return MenuItemResult::REDRAW;
        }
    };

    template <uint16_t UNITS_PER_INTEGER>
    int FixedPointItem<UNITS_PER_INTEGER>::valueTmp;
    
    class ReturnItem : public MenuItem
    {
    public:
        ReturnItem() : MenuItem(G_ARROW_UP G_ARROW_UP G_ARROW_UP G_ARROW_UP) {}
        void RenderCompact(FullLineWriter *lw, int line, bool invert) override
        {
            lw->printfl(line, invert, "%s", name);
        }

        OnOpenFullscreenResult OnOpenFullscreen() override { return OnOpenFullscreenResult::NOT_OK_AND_CLOSE_PREVIOUS_WINDOW; }
    };

    class PlaceholderItem : public MenuItem
    {
    public:
        PlaceholderItem(const char *const name) : MenuItem(name) {}
        void RenderCompact(FullLineWriter *lw, int line, bool invert) override
        {
            lw->printfl(line, invert, "%s", name);
        }
        OnOpenFullscreenResult OnOpenFullscreen() override { return OnOpenFullscreenResult::NOT_OK; }
    };

    class FolderItem : public MenuItem
    {
    private:
        const std::vector<MenuItem *> *const content;
        size_t selected_menu{0}; // index in the content vector
        size_t selected_line{0}; // index in the graphics ram o
        int movement{0};

    public:
        FolderItem(const char *const name, const std::vector<MenuItem *> *const content) : MenuItem(name), content(content) {}
        
        esp_err_t SaveToNvs(nvs_handle_t& nvs_handle) override{
            for (int i=0;i<content->size();i++) {
                content->at(i)->SaveToNvs(nvs_handle);
            }
            return ESP_OK;
        }

        esp_err_t OpenFromNvs(nvs_handle_t& nvs_handle) override{
            for (int i=0;i<content->size();i++) {
                content->at(i)->OpenFromNvs(nvs_handle);
            }
            return ESP_OK;
        }
        
        void RenderCompact(FullLineWriter *lw, int line, bool invert) override
        {
            lw->printfl(line, invert, "%s\t\t" G_CHEVRON_RIGHT, name);
        }

        MenuItem *GetContent(int uncorrected_index)
        {
            return content->at(modulo(uncorrected_index, content->size()));
        }

        void RenderFullScreen(FullLineWriter *lw, bool initial, uint8_t shownLines, uint8_t availableLines) override
        {
            if (initial)
            {
                ESP_LOGD(TAG, "Menu Full update selected_menu=%u, shownLines=%d availableLines=%d", selected_menu, shownLines, availableLines);
                lw->ClearScreenAndResetStartline(false, shownLines == availableLines ? 0 : 3, 4);
                // Bei zwei sichtbaren Zeilen wird von der selected_menu - 1 nur ein viertel und von der selected_menu+1 dafür drei viertel dargestellt
                selected_line = 1;
                int menu_idx = selected_menu - 1;
                for (int textline = 0; textline < availableLines; textline++)
                {
                    int corr_menu_idx = modulo(menu_idx, content->size());
                    MenuItem *itm = content->at(corr_menu_idx);
                    itm->RenderCompact(lw, textline, menu_idx == selected_menu);
                    menu_idx++;
                }
            }
            else if (movement == +1)
            {
                ESP_LOGD(TAG, "Update ram %d with %s and ram %d with %s and ram %d with %s",
                         modulo(selected_line, availableLines), GetContent(selected_menu)->GetName(),
                         modulo(selected_line + 1, availableLines), GetContent(selected_menu + 1)->GetName(),
                         modulo(selected_line + (availableLines - 1), availableLines), GetContent(selected_menu + availableLines - 1)->GetName());
                if (shownLines == availableLines)
                {
                    lw->printfl(selected_line - 1, false, " "); // delete first line on display (needs at least a space character; otherwise nothing will be printed)
                }
                GetContent(selected_menu)->RenderCompact(lw, selected_line, false);        // redraw selected line as unselected
                GetContent(selected_menu + 1)->RenderCompact(lw, selected_line + 1, true); // redraw next line as selected
                lw->Scroll(+1);
                GetContent(selected_menu + availableLines - 1)->RenderCompact(lw, selected_line + availableLines - 1, false);
                selected_line = modulo(++selected_line, availableLines);
                selected_menu = modulo(++selected_menu, content->size());
                movement = 0;
            }
        }

        //OBSOLETE
        void RenderFullScreen32(FullLineWriter *lw, bool initial, uint8_t textLines)
        {
            if (initial)
            {
                ESP_LOGI(TAG, "RenderFullScreen32: Menu Full update");
                lw->ClearScreenAndResetStartline(false, 16);
                selected_line = 3;
                int menu_idx = selected_menu - 3;
                for (int page = 0; page < 8; page++)
                {
                    int corr_menu_idx = modulo(menu_idx, content->size());
                    MenuItem *itm = content->at(corr_menu_idx);
                    itm->RenderCompact(lw, page, menu_idx == selected_menu);
                    menu_idx++;
                }
            }
            else if (movement == +1)
            {
                ESP_LOGD(TAG, "RenderFullScreen32: Update ram %d with %s and %d with %s and %d with %s",
                         modulo(selected_line, textLines), GetContent(selected_menu)->GetName(),
                         modulo(selected_line + 1, textLines), GetContent(selected_menu + 1)->GetName(),
                         modulo(selected_line + textLines - 1, textLines), GetContent(selected_menu + 5)->GetName());
                GetContent(selected_menu)->RenderCompact(lw, selected_line, false);
                GetContent(selected_menu + 1)->RenderCompact(lw, selected_line + 1, true);
                lw->Scroll(1);
                GetContent(selected_menu + 5)->RenderCompact(lw, selected_line + 5, false);
                selected_line = modulo(++selected_line, textLines);
                selected_menu = modulo(++selected_menu, content->size());
                movement = 0;
            }
        }

        MenuItemResult Up() override
        {
            movement = -1;
            return MenuItemResult::REDRAW;
        }
        MenuItemResult Down() override
        {
            movement = +1;
            return MenuItemResult::REDRAW;
        }

        MenuItemResult Select(MenuItem **itemToOpen) override
        {
            *itemToOpen = content->at(selected_menu);
            return MenuItemResult::OPEN_NEW_FULLSCREEN;
        }
    };

    class OptionItem : public MenuItem
    {
    private:
        std::vector<const char *> *options;
        size_t selectedOption{0}; // index in the content vector
        static uint32_t selectedOptionTmp;
        static uint32_t selectedRamlineTmp;   // index in the graphics ram o
        int movement{0};
        MenuItemChanged<uint32_t> *cb;

    public:
        OptionItem(const char *name, std::vector<const char *> *options, MenuItemChanged<uint32_t> *cb = nullptr) : MenuItem(name), options(options), cb(cb) {}
        
        esp_err_t SaveToNvs(nvs_handle_t& nvs_handle) override{
            return nvs_set_u32(nvs_handle, name, selectedOption);
        }

        esp_err_t OpenFromNvs(nvs_handle_t& nvs_handle) override{
            return nvs_get_u32(nvs_handle, name, (uint32_t*)&selectedOption);
        }

        void RenderCompact(FullLineWriter *lw, int page, bool invert) override
        {
            lw->printfl(page, invert, "%s\t\t%s", name, GetSelectedOptionName());
        }

        const char *GetSelectedOptionName(int offset = 0)
        {
            auto name = options->at(modulo(selectedOptionTmp + offset, options->size()));
            return name;
        }

        const char *GetSelectedOptionSymbol(int offsetWrtSelectedOptionTmp = 0)
        {
            return modulo(selectedOptionTmp+offsetWrtSelectedOptionTmp, options->size())==selectedOption?G_ARROW_LEFT_BOX:" ";
        }

        size_t GetSelectedOptionIndex()
        {
            return selectedOption;
        }

        void RenderFullScreen(FullLineWriter *lw, bool initial, uint8_t shownLines, uint8_t availableLines) override
        {
            if (initial)
            {
                lw->ClearScreenAndResetStartline(false, 0);
                selectedOptionTmp=selectedOption; //selectedOptionTemp wird auf dem Display immer in der zweiten dargestellten Zeile dargestellt
                selectedRamlineTmp=1;
                ESP_LOGD(TAG, "Full update, selectedRamlineTmp=%lu, selectedOptionTmp=%lu", selectedRamlineTmp, selectedOptionTmp);
                for (int ramline = 0; ramline < availableLines; ramline++)
                {
                    lw->printfl(ramline, ramline==1, "%s %s", GetSelectedOptionName(-1+ramline), GetSelectedOptionSymbol(- 1+ramline));
                }
            }
            else if (movement == +1)
            {
                ESP_LOGD(TAG, "Partial update start, selectedRamlineTmp=%lu, selectedOptionTmp=%lu", selectedRamlineTmp, selectedOptionTmp);
                lw->printfl(selectedRamlineTmp - 1, false, " ");                                    // delete first line on display (needs at least a space character; otherwise nothiong will be printed)
                lw->printfl(selectedRamlineTmp, false, "%s %s", GetSelectedOptionName(), GetSelectedOptionSymbol(0));           // redraw selected line as unselected
                lw->printfl(selectedRamlineTmp + 1, true, "%s %s", GetSelectedOptionName(+1), GetSelectedOptionSymbol(+1)); // redraw next line as selected
                lw->Scroll(+1);
                lw->printfl(selectedRamlineTmp + availableLines - 1, false, "%s %s", GetSelectedOptionName(availableLines - 1), GetSelectedOptionSymbol(availableLines - 1));
                selectedRamlineTmp = modulo(++selectedRamlineTmp, availableLines);
                selectedOptionTmp = modulo(++selectedOptionTmp, options->size());
                ESP_LOGD(TAG, "Partial update end, selectedRamlineTmp=%lu, selectedOptionTmp=%lu", selectedRamlineTmp, selectedOptionTmp);
                movement = 0;
            }
        }

         MenuItemResult Select(MenuItem **toOpen) override {
            selectedOption=selectedOptionTmp;
             if (cb)
                cb->ValueChanged(this, selectedOption);
            return MenuItemResult::CLOSE_MYSELF;
        }

        MenuItemResult Up() override
        {
            movement = -1;
            return MenuItemResult::REDRAW;
        }
        MenuItemResult Down() override
        {
            movement = +1;
            return MenuItemResult::REDRAW;
        }
    };

    // Management enthält ein Vector mit dem Pfad der Bearbeitung
    // MenuFolder enthält andere MenuFolder und auch MenuItems
    // Wird ein MenuItem selektiert, wird es dem Pfad hinzugefügt und es darf sich FullScreen zeigen
    // Normales Item wechselt es in eine Bearbeitungssicht Show. es bekommt dann alle Nutzerbefehle Up, Down, Select, Back

    class MenuManagement
    {
    private:
        FolderItem *root;
        FullLineWriter *lw;
        std::vector<MenuItem *> path;
        uint8_t shownLines;
        uint8_t availableLines;

    public:
        MenuManagement(FolderItem *root, FullLineWriter *lw) : root(root), lw(lw), shownLines(lw->GetShownLines()), availableLines(lw->GetAvailableLines()) {}

        void Init()
        {
            root->RenderFullScreen(lw, true, shownLines, availableLines);
            path.push_back(this->root);
        }

        void SaveToNvs(const char *partition_label, const char* namespace_name){
            esp_err_t err = nvs_flash_init_partition(partition_label);
            if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
                // NVS partition was truncated and needs to be erased
                // Retry nvs_flash_init
                ESP_ERROR_CHECK(nvs_flash_erase_partition(partition_label));
                err = nvs_flash_init_partition(partition_label);
            }
            ESP_ERROR_CHECK( err );
            ESP_LOGI(TAG, "Opening Non-Volatile Storage (NVS) handle... ");
            nvs_handle_t nvs_handle;
            err = nvs_open_from_partition(partition_label, namespace_name, NVS_READWRITE, &nvs_handle);
            this->root->SaveToNvs(nvs_handle);
            nvs_commit(nvs_handle);
        }

        void OpenFromNvs(const char *partition_label, const char* namespace_name){
            esp_err_t err = nvs_flash_init_partition(partition_label);
            if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
                // NVS partition was truncated and needs to be erased
                // Retry nvs_flash_init
                ESP_ERROR_CHECK(nvs_flash_erase_partition(partition_label));
                err = nvs_flash_init_partition(partition_label);
            }
            ESP_ERROR_CHECK( err );
            ESP_LOGI(TAG, "Opening Non-Volatile Storage (NVS) handle... ");
            nvs_handle_t nvs_handle;
            err = nvs_open_from_partition(partition_label, namespace_name, NVS_READONLY, &nvs_handle);
            this->root->OpenFromNvs(nvs_handle);
            nvs_commit(nvs_handle);
        }


        void Down()
        {
            MenuItem *mi = path.back();
            switch (mi->Down())
            {
            case MenuItemResult::REDRAW:
                mi->RenderFullScreen(lw, false, shownLines, availableLines);
                break;
            case MenuItemResult::NO_ACTION:
                break;
            default:
                break;
            }
        }

        void GoBack()
        {
            MenuItem *mi = path.back();
            mi->OnCloseFullscreen();
            path.pop_back();
            path.back()->OnOpenFullscreen();
            path.back()->RenderFullScreen(lw, true, shownLines, availableLines);
        }
        void Select()
        {
            MenuItem *toOpen{nullptr};
            MenuItem *mi = path.back();
            switch (mi->Select(&toOpen))
            {
            case MenuItemResult::NO_ACTION:
                break;
            case MenuItemResult::REDRAW:
                mi->RenderFullScreen(lw, false, shownLines, availableLines);
                break;
            case MenuItemResult::CLOSE_MYSELF:
                GoBack();
                break;
            case MenuItemResult::OPEN_NEW_FULLSCREEN:
            {
                assert(toOpen);
                switch (toOpen->OnOpenFullscreen())
                {
                case OnOpenFullscreenResult::OK:
                    mi->OnCloseFullscreen();
                    toOpen->RenderFullScreen(lw, true, shownLines, availableLines);
                    path.push_back(toOpen);
                    break;
                case OnOpenFullscreenResult::NOT_OK_AND_CLOSE_PREVIOUS_WINDOW:
                    GoBack();
                    break;
                case OnOpenFullscreenResult::NOT_OK:
                    break;
                default:
                    break;
                }
                break;
            }
            default:
                break;
            }
        }
    };
}
#undef TAG