#include "config.hpp"

#include <string>
#include <vector>

namespace Config::TimeTableMappingConfig {
    std::map<str, std::tuple<std::unordered_set<str>, std::unordered_set<str> > > personal_timetable_entries;
    std::map<int, std::tuple<str, str, std::unordered_set<str> > > teacher_mapping;
    std::map<std::tuple<str, str, int>, std::tuple<int, int, int> > subject_to_color;
    std::tuple<int, int, int> default_subject_color = {255, 255, 255};
}; // namespace Config::TimeTableMappingConfig

namespace Config::LanguageConfig {
    std::vector<std::pair<str, str> > weekday_name_mapping;
    str tomorrow;
    str today;
    str yesterday;
    str last_week;
    str next_week;
    str two_week_abbreviation;
    str two_week_abbreviation_legend;
    str class_timetable;
    str room_timetable;
    str teacher_timetable;
    str personal_timetable;
    str unknown_element_extended_text;
    str some_hour;
    str is_cancelled;
    str are_cancelled;
    str is_irregular;
    str are_irregular;
    str instead;
    str multiple_lessons_cancelled;
    str multiple_lessons_irregular;
    str back;
    str time;
    str unexpected_error;

    void set_internal_lang(const str &lang) {
        if (lang == "en") {
            weekday_name_mapping = {
                    {"Monday", "Monday"}, {"Tuesday", "Tuesday"}, {"Wednesday", "Wednesday"},
                    {"Thursday", "Thursday"}, {"Friday", "Friday"}, {"Saturday", "Saturday"},
                    {"Sunday", "Sunday"}
            };
            tomorrow = "Tomorrow";
            today = "Today";
            yesterday = "Yesterday";
            last_week = "Last Week";
            next_week = "Next Week";
            two_week_abbreviation = "(2W)";
            two_week_abbreviation_legend = "(2W) = Every 2 weeks";

            class_timetable = "Class Timetable";
            room_timetable = "Room Timetable";
            teacher_timetable = "Teacher Timetable";
            personal_timetable = "Timetable";

            unknown_element_extended_text = "Unknown";
            some_hour = "Some Hour";
            is_cancelled = "is cancelled";
            are_cancelled = "are cancelled";
            is_irregular = "is irregular";
            are_irregular = "are irregular";
            instead = "Instead of";

            multiple_lessons_cancelled = "Multiple lessons are cancelled";
            multiple_lessons_irregular = "Multiple lessons are irregular";

            back = "Back";
            time = "Time";

            unexpected_error = "Error! Contact the Administrator of the Bot! Time of the error";
        } else if (lang == "de") {
            weekday_name_mapping = {
                    {"Monday", "Montag"}, {"Tuesday", "Dienstag"}, {"Wednesday", "Mittwoch"},
                    {"Thursday", "Donnerstag"}, {"Friday", "Freitag"}, {"Saturday", "Samstag"},
                    {"Sunday", "Sonntag"}
            };
            tomorrow = "Morgen";
            today = "Heute";
            yesterday = "Gestern";
            last_week = "Letzte Woche";
            next_week = "Nächste Woche";
            two_week_abbreviation = "(2W)";
            two_week_abbreviation_legend = "(2W) = Alle 2 Wochen";

            class_timetable = "Stundenplan";
            room_timetable = "Raumplan";
            teacher_timetable = "Lehrerplan";
            personal_timetable = "Stundenplan";

            unknown_element_extended_text = "Unbekannt";
            some_hour = "Eine Stunde";
            is_cancelled = "entfällt";
            are_cancelled = "entfallen";
            is_irregular = "ist irregulär";
            are_irregular = "sind irregulär";
            instead = "Statt";

            multiple_lessons_cancelled = "Mehrere Stunden entfallen";
            multiple_lessons_irregular = "Mehrere Stunden sind irregulär";

            back = "Zurück";
            time = "Zeit";

            unexpected_error = "Fehler! Kontaktiere den Administrator des Bots! Zeit des Fehlers";
        }
    }
}; // namespace Config::LanguageConfig

namespace Config::HTMLStyleConfig {
    str timetable_html_header;
    str timetable_html_footer_two_week;
    str personal_timetable_html_header;
    str personal_timetable_html_footer;

    std::tuple<int, int, int> table_header_base_rgb = {32, 16, 102};
    std::tuple<int, int, int> today_personal_rgb_value = {21, 8, 79};

    str timetable_html_footer = "\n"
            "        <p style=\"text-align:center; font-size:20px; margin-top:10px;\">"
            "          powered by: IceWizard7"
            "        </p>";

    str unknown_element_symbol = "?";

    str lesson_time_ranges_format = "%H:%M";
    std::vector<str> lesson_time_ranges = {
            "07:50 - 08:40", "08:45 - 09:35", "09:40 - 10:30", "10:45 - 11:35",
            "11:40 - 12:30", "12:35 - 13:25", "13:30 - 14:20", "14:25 - 15:15",
            "15:20 - 16:10", "16:15 - 17:05", "17:10 - 18:00"
    };


    void set_internal_lang(const str &lang) {
        timetable_html_header = R"(
                    <!DOCTYPE html>
                    <html lang=)" +
                                lang + R"(>
                    <head>
                      <meta charset="UTF-8">
                      <meta name="viewport" content="width=device-width, initial-scale=1.0">
                      <title>)" +
                                LanguageConfig::personal_timetable + R"(</title>
                      <style>
                            body {
                                padding: 8px;          /* interior margin captured by the screenshot clip; tune to taste */
                                background: #fff;       /* ensure the padded area renders white in the PNG                */
                                box-sizing: border-box; /* keep body border-box width == viewport width                   */
                            }

                            @page {
                                size: 140mm 200mm; /* slightly smaller than A5 */
                                margin: 100mm;       /* adjust margin to taste */
                            }

                            p {
                                font-family: monospace;
                                text-align: center;
                                font-size: 13px;
                                margin-top: 10px;
                            }

                            table {
                                border-collapse: separate;
                                border-spacing: 0;
                                width: 100%;
                                table-layout: fixed;
                                font-size: 9pt; /* slightly smaller font to fit portrait */
                                font-family: monospace;
                                box-shadow: 0 0 10px rgba(0,0,0,0.1);
                            }

                            th, td {
                                border: 1px solid #ddd;
                                padding: 8px;
                                text-align: center;
                            }

                            td {
                                text-align: center;
                                vertical-align: middle;
                                position: relative;
                                /*background: linear-gradient(to right,
                                    var(--stripe-color, transparent) 0 6%,
                                    transparent 6% 100%);*/
                                overflow: hidden;
                                height: 40px;
                                z-index: 0; /* stacking context */
                            }

                            tr:nth-child(even) {
                                background-color: #f9f9f9;
                            }

                            th {
                                color: white;
                                font-size: 11pt;
                            }

                            /* Create a triangle in the bottom-right corner */
                            td::after {
                                content: "";
                                position: absolute;
                                right: 0;
                                bottom: 0;
                                width: 0;
                                height: 0;
                                border-style: solid;
                                border-width: 0 0 20px 20px; /* size of the triangle */
                                border-color: transparent transparent var(--stripe-color, transparent) transparent;
                                z-index: 0; /* behind text */
                            }

                            td > * {
                                position: relative;
                                z-index: 1; /* above triangle */
                            }
                        </style>
                    </head>
                    <body>
                    )";

        timetable_html_footer_two_week = R"(
            <p style="text-align:center; font-size:15px; margin-top:10px;">
              )" + LanguageConfig::two_week_abbreviation_legend +
                                         R"(
            </p>
            )";

        personal_timetable_html_header = R"(
                    <!DOCTYPE html>
                    <html lang=)" + lang +
                                         R"(>
                    <head>
                      <meta charset="UTF-8">
                      <meta name="viewport" content="width=device-width, initial-scale=1.0">
                      <title>)" + LanguageConfig::personal_timetable +
                                         R"(</title>
                      <style>
                            @page {
                                size: 140mm 200mm;
                                margin: 20mm;
                            }

                            * {
                                box-sizing: border-box;
                            }

                            html {
                                height: 100%;
                            }

                            body {
                                font-family: monospace;
                                min-height: 100vh;
                                margin: 0;
                                display: flex;
                                flex-direction: column;
                            }

                            .personal-timetable-page {
                                flex: 1 1 auto;
                                min-height: 0;
                                width: 100%;
                                display: flex;
                                flex-direction: column;
                                gap: 12px;
                                padding: 12px;
                            }

                            .personal-timetable-nav {
                                flex: 0 0 auto;
                                text-align: center;
                                font-size: 13px;
                            }

                            .personal-timetable-nav-row {
                                display: grid;
                                grid-template-columns: auto minmax(0, 1fr) auto;
                                align-items: center;
                                gap: 8px;
                            }

                            .personal-timetable-nav-row a:first-child {
                                justify-self: start;
                            }

                            .personal-timetable-nav-row a:last-child {
                                justify-self: end;
                            }

                            .personal-timetable-title {
                                min-width: 0;
                                text-align: center;
                                overflow-wrap: anywhere;
                            }

                            .personal-timetable-today-row {
                                margin-top: 8px;
                            }

                            .personal-timetable-table-wrap {
                                flex: 1 1 auto;
                                min-height: 0;
                                width: 100%;
                                overflow: auto;
                            }

                            table {
                                border-collapse: separate;
                                border-spacing: 0;
                                table-layout: fixed;
                                font-size: 10pt;
                                width: 100%;
                                height: 100%;
                                box-shadow: 0 0 10px rgba(0,0,0,0.1);
                            }

                            th, td {
                                border: 1px solid #ddd;
                                padding: 6px 10px;
                                text-align: center;
                            }

                            th {
                                color: white;
                                font-size: 10pt;
                            }

                            td {
                                vertical-align: middle;
                                position: relative;
                                overflow: hidden;
                                z-index: 0;
                            }

                            td::after {
                                content: "";
                                position: absolute;
                                right: 0;
                                bottom: 0;
                                width: 0;
                                height: 0;
                                border-style: solid;
                                border-width: 0 0 16px 16px;
                                border-color: transparent transparent var(--stripe-color, transparent) transparent;
                                z-index: 0;
                            }

                            td > * {
                                position: relative;
                                z-index: 1;
                            }
                          </style>
                        </head>
                        <body>
                    )";
    }
}; // namespace Config::HTMLStyleConfig

namespace Config {
    Renderer renderer;

    void set_lang(const str &lang) {
        LanguageConfig::set_internal_lang(lang);
        HTMLStyleConfig::set_internal_lang(lang);
    }
}; // namespace Config
