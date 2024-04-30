class : public MenuItemChanged<int>
{
public:
    void ValueChanged(const MenuItem *item, int newValue) override
    {
        ESP_LOGI(TAG, "Value of '%s' changed to %d", item->GetName(), newValue);
    }
} int_cb;

class : public MenuItemChanged<bool>
{
public:
    void ValueChanged(const MenuItem *item, bool newValue) override
    {
        ESP_LOGI(TAG, "Value of '%s' changed to %d", item->GetName(), newValue);
    }
} bool_cb;

class : public MenuItemChanged<float>
{
public:
    void ValueChanged(const MenuItem *item, float newValue) override
    {
        ESP_LOGI(TAG, "Value of '%s' changed to %f", item->GetName(), newValue);
    }
} float_cb;

int Integer2_1_0{1};
int Integer2_0{2};
int Integer2_2{3};
int Integer2_3{4};
int Integer0{0};
int Integer5{5};
bool Bool3{false};
float Fixed1{0.25};

std::vector<MenuItem *> folder2_1_items = {
    new IntegerItem("Integer2_1_0", &Integer2_1_0, 0, 10, &int_cb),
    new ReturnItem(),
};

std::vector<MenuItem *> folder2_items = {
    new IntegerItem("Integer2_0", &Integer2_0, 0, 10, &int_cb),
    new FolderItem("Folder2_1", &folder2_1_items),
    new IntegerItem("Integer2_2", &Integer2_2, 0, 10, &int_cb),
    new IntegerItem("Integer2_3", &Integer2_3, 0, 10, &int_cb),
    new ReturnItem(),
};

std::vector<const char *> option4_items = {
    "Opt1",
    "Opt2",
    "Opt3",
    "Opt4",
    "Opt5",
};

std::vector<const char *> option6_items = {
    G_CAMERA "Cam",
    G_BOOK "Book",
    G_MOVIE "Movie",
    G_WIND "Wind",
    G_CAR "Car",
};

std::vector<MenuItem *> root_items = {
    new IntegerItem("Integer0", &Integer0, 0, 10, &int_cb),
    new FixedPointItem<4>("Fixed1", &Fixed1, 0, 1, &float_cb),
    new FolderItem("Folder2", &folder2_items),
    new BoolItem("Bool3", &Bool3, &bool_cb),
    new OptionItem("Option4", &option4_items, &int_cb),
    new IntegerItem("Integer5", &Integer5, 0, 10, &int_cb),
    new OptionItem("Option6", &option6_items, &int_cb),
    new PlaceholderItem("Ph7\t***\tPh7"),
};

enum class ButtonPressResult
{
    NO_CHANGE,
    PRESSED,
    RELEASED_SHORT,
    PRESSED_LONG,
    RELEASED_LONG,
};

time_t pressed_time{INT64_MAX};
bool pressed_state{true};
bool pressed_long_already_sent{false};

static ButtonPressResult Button()
{
    bool newstate = gpio_get_level(GPIO_NUM_0);
    time_t now = esp_timer_get_time();
    if (pressed_state && !newstate)
    { // pressed down
        pressed_state = newstate;
        pressed_time = esp_timer_get_time();
        pressed_long_already_sent = false;
        ESP_LOGD(TAG, "Pressed!");
        return ButtonPressResult::PRESSED;
    }
    else if (!pressed_state && !newstate && !pressed_long_already_sent && now - pressed_time > 400 * 1000)
    {
        pressed_state = newstate;
        pressed_long_already_sent = true;
        ESP_LOGD(TAG, "Pressed long!");
        return ButtonPressResult::PRESSED_LONG;
    }
    else if (!pressed_state && newstate)
    {
        pressed_state = newstate;
        pressed_time = INT64_MAX;
        ESP_LOGD(TAG, "Released!");
        return pressed_long_already_sent ? ButtonPressResult::RELEASED_LONG : ButtonPressResult::RELEASED_SHORT;
    }
    return ButtonPressResult::NO_CHANGE;
}