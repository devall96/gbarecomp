#pragma once

#include <fstream>
#include <string>
#include <utility>
#include <vector>

namespace gbarecomp {

// Replace only the named keys in config.ini [Launcher]. Preserve controller
// bindings, save paths and every other launcher setting in the same INI file.
inline void save_launcher_keys(const std::string& path,
                               const std::vector<std::pair<std::string, std::string>>& keys) {
    if (path.empty() || keys.empty()) return;
    std::vector<std::string> lines;
    std::ifstream input(path);
    std::string line;
    bool launcher = false;
    while (std::getline(input, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        const auto start = line.find_first_not_of(" \t");
        if (start != std::string::npos && line[start] == '[')
            launcher = line.compare(start, 10, "[Launcher]") == 0;
        if (launcher && start != std::string::npos) {
            const auto eq = line.find('=', start);
            if (eq != std::string::npos) {
                const auto end = line.find_last_not_of(" \t", eq - 1);
                const auto key = line.substr(start, end - start + 1);
                bool replaced = false;
                for (const auto& kv : keys) replaced = replaced || kv.first == key;
                if (replaced) continue;
            }
        }
        lines.push_back(line);
    }
    input.close();
    std::ofstream output(path, std::ios::trunc);
    if (!output) return;
    auto write_keys = [&] { for (const auto& kv : keys) output << kv.first << " = " << kv.second << '\n'; };
    bool inserted = false;
    for (const auto& kept : lines) {
        output << kept << '\n';
        const auto start = kept.find_first_not_of(" \t");
        if (!inserted && start != std::string::npos && kept.compare(start, 10, "[Launcher]") == 0) {
            write_keys();
            inserted = true;
        }
    }
    if (!inserted) {
        output << "\n[Launcher]\n";
        write_keys();
    }
}

// Update only the three original presentation choices.
inline void save_presentation_settings(const std::string& path, bool integer_scale,
                                       bool scanlines, int strength) {
    save_launcher_keys(path, {{"integer_scale", std::to_string(int(integer_scale))},
                              {"scanlines", std::to_string(int(scanlines))},
                              {"scanline_strength", std::to_string(strength)}});
}

} // namespace gbarecomp
