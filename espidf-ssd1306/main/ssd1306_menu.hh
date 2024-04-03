#pragma once

#include <cstdint>
#include <cstdio>
#include <ssd1306.hh>

namespace ssd1306
{
    namespace menu
    {
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

        class MenuItem
        {
        protected:
            const char *name;

        public:
            MenuItem(const char *name) : name(name) {}
            virtual void RenderCompact(FullLineWriter *lw, int page_modulo_inside, bool invert) = 0;
            virtual void RenderFullScreen(FullLineWriter *lw, bool initial, uint8_t textLines){};
            virtual OnOpenFullscreenResult OnOpenFullscreen() { return OnOpenFullscreenResult::OK; } // This is the normal case
            virtual void OnCloseFullscreen() { return; }                                             // This is the normal case
            virtual MenuItemResult Up() { return MenuItemResult::CLOSE_MYSELF; };
            virtual MenuItemResult Down() { return MenuItemResult::CLOSE_MYSELF; };
            virtual MenuItemResult Select(MenuItem **itemToOpen) { return MenuItemResult::CLOSE_MYSELF; };
            virtual MenuItemResult Back() { return MenuItemResult::CLOSE_MYSELF; };
            const char *GetName() { return name; }
        };

        class IntegerItem : public MenuItem
        {
        private:
            int value, value_min, value_max;

        public:
            IntegerItem(const char *name, int value_init, int value_min, int value_max) : MenuItem(name), value(value_init), value_min(value_min), value_max(value_max) {}
            void RenderCompact(FullLineWriter *lw, int line, bool invert) override
            {
                lw->printfl(line, invert, "%s\t%d", name, value);
            }

            void RenderFullScreen(FullLineWriter *lw, bool initial, uint8_t screenHeight) override
            {
                if (initial)
                {
                    lw->ClearScreenAndResetStartline();
                    lw->printfl(0, false, "Edit Value");
                }
                lw->printfl(1, true, "%d", value);
            }
            MenuItemResult Up() override
            {
                value++;
                if (value > value_max)
                    value = value_min;
                return MenuItemResult::REDRAW;
            }
            MenuItemResult Down() override
            {
                value--;
                if (value < value_min)
                    value = value_max;
                return MenuItemResult::REDRAW;
            }
        };

        class BoolItem : public MenuItem
        {
        private:
            bool value;

        public:
            BoolItem(const char *name, bool value_init) : MenuItem(name), value(value_init) {}
            void RenderCompact(FullLineWriter *lw, int line, bool invert) override
            {
                if (value)
                {
                    lw->printfl(line, invert, "%s\t\x1b\x10", name);
                }
                else
                {
                    lw->printfl(line, invert, "%s\t\x1b\xf", name);
                }
            }

            void RenderFullScreen(FullLineWriter *lw, bool initial, uint8_t screenHeight) override
            {
                if (initial)
                {
                    lw->ClearScreenAndResetStartline();
                    lw->printfl(0, false, "Edit Value");
                }
                if (value)
                {
                    lw->printfl(1, true, "\x1b\x10");
                }
                else
                {
                    lw->printfl(1, true, "\x1b\xf");
                }
            }

            MenuItemResult Up() override
            {
                value = !value;
                return MenuItemResult::REDRAW;
            }
            MenuItemResult Down() override
            {
                value = !value;
                return MenuItemResult::REDRAW;
            }
        };

        template <uint16_t UNITS_PER_INTEGER = 10>
        class FixedPointItem : public MenuItem
        {
        private:
            int value, value_min, value_max;

        public:
            FixedPointItem(const char *name, float value_init, float value_min, float value_max) : MenuItem(name), value(value_init * UNITS_PER_INTEGER), value_min(value_min * UNITS_PER_INTEGER), value_max(value_max * UNITS_PER_INTEGER) {}
            void RenderCompact(FullLineWriter *lw, int line, bool invert) override
            {
                lw->printfl(line, invert, "%s\t%.2f", name, (float)value / (float)UNITS_PER_INTEGER);
            }

            void RenderFullScreen(FullLineWriter *lw, bool initial, uint8_t screenHeight) override
            {
                if (initial)
                {
                    lw->ClearScreenAndResetStartline();
                    lw->printfl(0, false, "Edit Value");
                }
                lw->printfl(1, true, "%.2f", (float)value / (float)UNITS_PER_INTEGER);
            }

            MenuItemResult Up() override
            {
                value++;
                if (value > value_max)
                    value = value_min;
                return MenuItemResult::REDRAW;
            }
            MenuItemResult Down() override
            {
                value--;
                if (value < value_min)
                    value = value_max;
                return MenuItemResult::REDRAW;
            }
        };

        class ReturnItem : public MenuItem
        {
        private:
        public:
            ReturnItem() : MenuItem("\x1b\x2\x1b\x2\x1b\x2\x1b\x2") {}
            void RenderCompact(FullLineWriter *lw, int line, bool invert) override
            {
                lw->printfl(line, invert, "%s", name);
            }

            OnOpenFullscreenResult OnOpenFullscreen() override { return OnOpenFullscreenResult::NOT_OK_AND_CLOSE_PREVIOUS_WINDOW; }
        };

        class PlaceholderItem : public MenuItem
        {
        public:
            PlaceholderItem(const char *name) : MenuItem(name) {}
            void RenderCompact(FullLineWriter *lw, int line, bool invert) override
            {
                lw->printfl(line, invert, "%s", name);
            }
            OnOpenFullscreenResult OnOpenFullscreen() override { return OnOpenFullscreenResult::NOT_OK; }
        };

        class FolderItem : public MenuItem
        {
        private:
            std::vector<MenuItem *> *content;
            size_t selected_menu{0}; // index in the content vector
            size_t selected_line{0}; // index in the graphics ram o
            int movement{0};

        public:
            FolderItem(const char *name, std::vector<MenuItem *> *content) : MenuItem(name), content(content) {}
            void RenderCompact(FullLineWriter *lw, int page, bool invert) override
            {
                lw->printfl(page, invert, "%s\t\x1b\x3", name);
            }

            MenuItem *GetContent(int uncorrected_index)
            {
                return content->at(ssd1306::modulo(uncorrected_index, content->size()));
            }

            void RenderFullScreen(FullLineWriter *lw, bool initial, uint8_t textLines) override
            {
                if (initial)
                {
                    ESP_LOGI(TAG, "Menu Full update");
                    lw->ClearScreenAndResetStartline(false, 0);
                    selected_line = 1;
                    int menu_idx = selected_menu - 1;
                    for (int textline = 0; textline < textLines; textline++)
                    {
                        int corr_menu_idx = ssd1306::modulo(menu_idx, content->size());
                        MenuItem *itm = content->at(corr_menu_idx);
                        itm->RenderCompact(lw, textline, menu_idx == selected_menu);
                        menu_idx++;
                    }
                }
                else if (movement == +1)
                {
                    ESP_LOGD(TAG, "Update ram %d with %s and %d with %s and %d with %s",
                             ssd1306::modulo(selected_line, textLines), GetContent(selected_menu)->GetName(),
                             ssd1306::modulo(selected_line + 1, textLines), GetContent(selected_menu + 1)->GetName(),
                             ssd1306::modulo(selected_line + (textLines - 1), textLines), GetContent(selected_menu + 5)->GetName());
                    lw->printfl(selected_line - 1, false, " ");                                // delete first line on display (needs at least a space character; otherwise nothiong will be printed)
                    GetContent(selected_menu)->RenderCompact(lw, selected_line, false);        // redraw selected line as unselected
                    GetContent(selected_menu + 1)->RenderCompact(lw, selected_line + 1, true); // redraw next line as selected
                    lw->Scroll(+1);
                    GetContent(selected_menu + textLines - 1)->RenderCompact(lw, selected_line + textLines - 1, false);
                    selected_line = ssd1306::modulo(++selected_line, textLines);
                    selected_menu = ssd1306::modulo(++selected_menu, content->size());
                    movement = 0;
                }
            }

            void RenderFullScreen32(FullLineWriter *lw, bool initial, uint8_t textLines)
            {
                if (initial)
                {
                    ESP_LOGI(TAG, "Menu Full update");
                    lw->ClearScreenAndResetStartline(false, 16);
                    selected_line = 3;
                    int menu_idx = selected_menu - 3;
                    for (int page = 0; page < 8; page++)
                    {
                        int corr_menu_idx = ssd1306::modulo(menu_idx, content->size());
                        MenuItem *itm = content->at(corr_menu_idx);
                        itm->RenderCompact(lw, page, menu_idx == selected_menu);
                        menu_idx++;
                    }
                }
                else if (movement == +1)
                {
                    ESP_LOGD(TAG, "Update ram %d with %s and %d with %s and %d with %s",
                             ssd1306::modulo(selected_line, textLines), GetContent(selected_menu)->GetName(),
                             ssd1306::modulo(selected_line + 1, textLines), GetContent(selected_menu + 1)->GetName(),
                             ssd1306::modulo(selected_line + textLines - 1, textLines), GetContent(selected_menu + 5)->GetName());
                    GetContent(selected_menu)->RenderCompact(lw, selected_line, false);
                    GetContent(selected_menu + 1)->RenderCompact(lw, selected_line + 1, true);
                    lw->Scroll(1);
                    GetContent(selected_menu + 5)->RenderCompact(lw, selected_line + 5, false);
                    selected_line = ssd1306::modulo(++selected_line, textLines);
                    selected_menu = ssd1306::modulo(++selected_menu, content->size());
                    movement = 0;
                }
            }

            MenuItemResult Up() override
            {
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
            size_t selected_option{0}; // index in the content vector
            size_t selected_line{0};   // index in the graphics ram o
            int movement{0};

        public:
            OptionItem(const char *name, std::vector<const char *> *options) : MenuItem(name), options(options) {}
            void RenderCompact(FullLineWriter *lw, int page, bool invert) override
            {
                lw->printfl(page, invert, "%s\t %s", name, GetSelectedOptionName());
            }

            const char *GetSelectedOptionName(int offset = 0)
            {
                auto name = options->at(ssd1306::modulo(selected_option + offset, options->size()));
                ESP_LOGI(TAG, "%s", name);
                return name;
            }

            size_t GetSelectedOptionIndex()
            {
                return selected_option;
            }

            void RenderFullScreen(FullLineWriter *lw, bool initial, uint8_t textLines) override
            {
                if (initial)
                {
                    ESP_LOGI(TAG, "Menu Full update");
                    lw->ClearScreenAndResetStartline(false, 0);
                    selected_line = 1;
                    int option_idx = selected_option - 1;
                    for (int textline = 0; textline < textLines; textline++)
                    {
                        const char *itm = options->at(ssd1306::modulo(option_idx, options->size()));
                        if (option_idx == selected_option)
                        {
                            lw->printfl(textline, true, "\x1b\xe %s", itm);
                        }
                        else
                        {
                            lw->printfl(textline, false, "   %s", itm);
                        }
                        option_idx++;
                    }
                }
                else if (movement == +1)
                {
                    lw->printfl(selected_line - 1, false, " ");                                    // delete first line on display (needs at least a space character; otherwise nothiong will be printed)
                    lw->printfl(selected_line, false, "   %s", GetSelectedOptionName());           // redraw selected line as unselected
                    lw->printfl(selected_line + 1, true, "\x1b\xe %s", GetSelectedOptionName(+1)); // redraw next line as selected
                    lw->Scroll(+1);
                    lw->printfl(selected_line + textLines - 1, false, "   %s", GetSelectedOptionName(textLines - 1));
                    selected_line = ssd1306::modulo(++selected_line, textLines);
                    selected_option = ssd1306::modulo(++selected_option, options->size());
                    movement = 0;
                }
            }

            MenuItemResult Up() override
            {
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
            uint8_t textLines{4};

        public:
            MenuManagement(FolderItem *root, FullLineWriter *lw) : root(root), lw(lw) {}

            void Init()
            {
                root->RenderFullScreen(lw, true, this->textLines);
                path.push_back(this->root);
            }

            void Down()
            {
                MenuItem *mi = path.back();
                switch (mi->Down())
                {
                case MenuItemResult::REDRAW:
                    mi->RenderFullScreen(lw, false, this->textLines);
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
                path.back()->RenderFullScreen(lw, true, this->textLines);
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
                    mi->RenderFullScreen(lw, false, this->textLines);
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
                        toOpen->RenderFullScreen(lw, true, this->textLines);
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
}