#pragma once

namespace Render_Utils {
    // Render after the navigation links: only the browser knows the request URL.
    // Update href itself so opening/copying links also preserves the parameters.
    inline constexpr const char *date_navigation_script = R"(<script>
document.querySelectorAll('a[href^="?date="]').forEach(link => {
    const target = new URL(link.getAttribute('href'), window.location.href);
    const url = new URL(window.location.href);
    url.searchParams.set('date', target.searchParams.get('date'));
    link.href = url.href;
});
</script>)";
}
