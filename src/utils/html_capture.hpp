#pragma once

#include "types.hpp"

namespace Render_Utils {
    inline str prepare_html_for_capture(const str &html) {
        // Interactive calendars scroll and select a single day on phones. A static
        // PNG must instead include every day and the entire time range.
        str capture_html = html;
        if (html.find("data-calendar-view=\"v2\"") != str::npos) {
            const auto head_end = capture_html.find("</head>");
            if (head_end != str::npos) {
                capture_html.insert(head_end, R"(<style data-untis-capture>
body{width:max-content;min-width:100%}main{max-width:none;height:auto;display:block}
.calendar-scroll{max-height:none!important;overflow:visible!important}
.calendar-grid{grid-template-columns:52px var(--days)!important}
.day-heading,.calendar-day{display:block!important}
.day-heading,.time-axis{position:relative!important}
.day-choice,.calendar-tabs,.day-tab{display:none!important}
</style>)");
            }
        }
        return capture_html;
    }
}
