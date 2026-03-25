#include "tui.h"
#include "network.h"
#include "phone.h"
#include "phone_cdc.h"
#include "reverse_phone.h"
#include "shout_phone.h"
#include "whisper_phone.h"

#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

#include <memory>
#include <sstream>
#include <string>
#include <vector>

using namespace ftxui;

void Tui::run() {
    std::unique_ptr<Network> network{};
    std::vector<PhoneCDC> transmission_log{};
    std::vector<std::string> phone_names{};
    std::vector<std::string> phone_types{"Phone", "ShoutPhone", "WhisperPhone", "ReversePhone"};
    std::vector<std::string> tab_labels{"Network", "Send", "State", "Log", "Exit"};

    std::string network_name;
    std::string phone_name_input;
    int phone_type_selected = 0;
    int subscribe_source = 0;
    int subscribe_target = 0;
    int send_phone_selected = 0;
    std::string message_input;
    std::string status_text;
    int tab_selected = 0;

    auto screen = ScreenInteractive::Fullscreen();

    auto network_name_input = Input(&network_name, "network name");
    auto phone_name_field = Input(&phone_name_input, "phone name");
    auto phone_type_radio = Radiobox(&phone_types, &phone_type_selected);
    auto message_field = Input(&message_input, "message");

    auto create_network_btn = Button("Create Network", [&] {
        if (network_name.empty()) {
            status_text = "Network name cannot be empty";
            return;
        }
        network = std::make_unique<Network>(network_name);
        phone_names.clear();
        transmission_log.clear();
        status_text = "Created network: " + network_name;
        network_name.clear();
    });

    auto add_phone_btn = Button("Add Phone", [&] {
        if (!network) {
            status_text = "Create a network first";
            return;
        }
        if (phone_name_input.empty()) {
            status_text = "Phone name cannot be empty";
            return;
        }

        const auto &type = phone_types[phone_type_selected];
        std::unique_ptr<Phone> phone;
        if (type == "ShoutPhone") {
            phone = std::make_unique<ShoutPhone>(phone_name_input);
        } else if (type == "WhisperPhone") {
            phone = std::make_unique<WhisperPhone>(phone_name_input);
        } else if (type == "ReversePhone") {
            phone = std::make_unique<ReversePhone>(phone_name_input);
        } else {
            phone = std::make_unique<Phone>(phone_name_input);
        }

        auto &ref = network->addPhone(std::move(phone));
        ref.onMessageTransmission([&](const PhoneCDC &cdc) { transmission_log.push_back(cdc); });
        phone_names.push_back(phone_name_input);
        status_text = "Added " + type + ": " + phone_name_input;
        phone_name_input.clear();
    });

    auto subscribe_btn = Button("Subscribe", [&] {
        if (!network || phone_names.size() < 2) {
            status_text = "Need at least 2 phones";
            return;
        }
        auto *source = network->findPhone(phone_names[subscribe_source]);
        auto *target = network->findPhone(phone_names[subscribe_target]);
        if (source == nullptr || target == nullptr) {
            status_text = "Phone not found";
            return;
        }
        if (source == target) {
            status_text = "Cannot subscribe to self";
            return;
        }
        source->addSubscriber(target);
        status_text = phone_names[subscribe_source] + " -> " + phone_names[subscribe_target];
    });

    auto send_btn = Button("Send", [&] {
        if (!network || phone_names.empty()) {
            status_text = "No phones available";
            return;
        }
        if (message_input.empty()) {
            status_text = "Message cannot be empty";
            return;
        }
        auto *phone = network->findPhone(phone_names[send_phone_selected]);
        if (phone == nullptr) {
            status_text = "Phone not found";
            return;
        }
        try {
            phone->sendMessage(message_input);
            status_text = "Sent from " + phone_names[send_phone_selected];
        } catch (const std::runtime_error &e) {
            status_text = std::string("Error: ") + e.what();
        }
        message_input.clear();
    });

    auto clear_log_btn = Button("Clear Log", [&] {
        transmission_log.clear();
        status_text = "Log cleared";
    });

    auto exit_btn = Button("Quit", screen.ExitLoopClosure());

    auto source_dropdown = Dropdown(&phone_names, &subscribe_source);
    auto target_dropdown = Dropdown(&phone_names, &subscribe_target);
    auto send_dropdown = Dropdown(&phone_names, &send_phone_selected);

    auto tab_network = Container::Vertical({
        Container::Horizontal({network_name_input, create_network_btn}),
        Container::Horizontal({phone_name_field, add_phone_btn}),
        phone_type_radio,
        Container::Horizontal({source_dropdown, target_dropdown, subscribe_btn}),
    });

    auto tab_send = Container::Vertical({
        send_dropdown,
        Container::Horizontal({message_field, send_btn}),
    });

    auto tab_state = Renderer([&] {
        if (!network) {
            return text("No network created") | center;
        }
        std::ostringstream oss;
        oss << *network;
        return paragraph(oss.str());
    });

    auto tab_log = Container::Vertical({
        Renderer([&] {
            if (transmission_log.empty()) {
                return text("No transmissions recorded") | center;
            }
            Elements entries;
            for (const auto &cdc : transmission_log) {
                std::ostringstream oss;
                oss << cdc;
                entries.push_back(paragraph(oss.str()));
                entries.push_back(separator());
            }
            return vbox(std::move(entries)) | yframe;
        }),
        clear_log_btn,
    });

    auto tab_exit = Container::Vertical({exit_btn});

    auto tab_menu = Menu(&tab_labels, &tab_selected, MenuOption::HorizontalAnimated());

    auto tab_content =
        Container::Tab({tab_network, tab_send, tab_state, tab_log, tab_exit}, &tab_selected);

    auto main_container = Container::Vertical({tab_menu, tab_content});

    auto renderer = Renderer(main_container, [&] {
        std::string help = "h/l:tabs  j/k:navigate  q:quit  1-5:jump";
        return vbox({
                   text("Phone Network Simulator") | bold | center,
                   separator(),
                   tab_menu->Render(),
                   separator(),
                   tab_content->Render() | flex,
                   separator(),
                   hbox({text(status_text) | flex, text(help) | dim}),
               }) |
               border;
    });

    renderer |= CatchEvent([&](const Event &event) {
        bool any_input_focused = network_name_input->Focused() || phone_name_field->Focused() ||
                                 message_field->Focused();

        if (any_input_focused) {
            return false;
        }

        if (event == Event::Character('q')) {
            screen.Exit();
            return true;
        }
        if (event == Event::Character('j')) {
            return main_container->OnEvent(Event::ArrowDown);
        }
        if (event == Event::Character('k')) {
            return main_container->OnEvent(Event::ArrowUp);
        }
        if (event == Event::Character('h')) {
            tab_selected = std::max(0, tab_selected - 1);
            return true;
        }
        if (event == Event::Character('l')) {
            tab_selected = std::min(static_cast<int>(tab_labels.size()) - 1, tab_selected + 1);
            return true;
        }
        if (event.is_character() && event.character()[0] >= '1' && event.character()[0] <= '5') {
            tab_selected = event.character()[0] - '1';
            return true;
        }
        return false;
    });

    screen.Loop(renderer);
}
