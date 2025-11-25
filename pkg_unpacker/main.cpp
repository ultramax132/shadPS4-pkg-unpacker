// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <iostream>
#include <filesystem>
#include <string>
#include <vector>
#include <iomanip>
#include "../src/core/file_format/pkg.h"

namespace fs = std::filesystem;

void PrintUsage(const char* program_name) {
    std::cout << "PS4 PKG Unpacker for shadPS4\n";
    std::cout << "Usage: " << program_name << " <pkg_file>\n\n";
    std::cout << "Arguments:\n";
    std::cout << "  pkg_file          Path to the .pkg file to unpack\n";
    std::cout << "Example:\n";
    std::cout << "  " << program_name << " game.pkg\n";
    std::cout << "\nNote: Output directory will be created in the same folder as the PKG file\n";
    std::cout << "      using the Title ID as the directory name.\n";
}

bool FindEboot(const fs::path& search_dir, fs::path& found_path) {
    for (const auto& entry : fs::recursive_directory_iterator(search_dir)) {
        if (entry.is_regular_file() && entry.path().filename() == "eboot.bin") {
            found_path = entry.path();
            return true;
        }
    }
    return false;
}

int main(int argc, char* argv[]) {
    if (argc < 2 || argc > 2) {
        PrintUsage(argv[0]);
        return 1;
    }

    fs::path pkg_path = argv[1];
    
    // Validate input file
    if (!fs::exists(pkg_path)) {
        std::cerr << "Error: PKG file not found: " << pkg_path << std::endl;
        return 1;
    }

    if (!fs::is_regular_file(pkg_path)) {
        std::cerr << "Error: Not a regular file: " << pkg_path << std::endl;
        return 1;
    }

    

    std::cout << "PS4 PKG Unpacker\n";
    std::cout << "================\n";
    std::cout << "Input file:  " << pkg_path << "\n";

    // Open and extract PKG
    PKG pkg;
    std::string failreason;

    std::cout << "Opening PKG file...\n";
    if (!pkg.Open(pkg_path, failreason)) {
        std::cerr << "Error: Failed to open PKG file: " << failreason << std::endl;
        return 1;
    }

    const bool is_patch_pkg = pkg.IsPatchPkg();
    std::string title_id(pkg.GetTitleID());
    if (title_id.empty()) {
        std::cerr << "Error: Failed to determine Title ID from PKG." << std::endl;
        return 1;
    }

    fs::path base_output_dir = pkg_path.parent_path() / title_id;
    fs::path output_dir = base_output_dir;
    if (is_patch_pkg) {
        if (fs::exists(base_output_dir)) {
            std::cout << "Detected patch PKG. Applying directly to existing base directory: "
                      << base_output_dir << "\n";
        } else {
            std::cout << "Detected patch PKG but base directory not found. Creating new base directory: "
                      << base_output_dir << "\n";
        }
    } else {
        std::cout << "Detected base/DLC PKG.\n";
    }

    std::cout << "Output dir:  " << output_dir << "\n\n";

    // Create output directory
    try {
        fs::create_directories(output_dir);
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Error: Failed to create output directory: " << e.what() << std::endl;
        return 1;
    }

    std::cout << "Title ID:    " << pkg.GetTitleID() << "\n";
    std::cout << "PKG Size:    " << std::fixed << std::setprecision(2) 
              << (pkg.GetPkgSize() / (1024.0 * 1024.0)) << " MB\n";
    std::cout << "Flags:       " << pkg.GetPkgFlags() << "\n\n";

    std::cout << "Extracting metadata (sce_sys)...\n";
    if (!pkg.Extract(pkg_path, output_dir, failreason)) {
        std::cerr << "Error: Failed to extract PKG: " << failreason << std::endl;
        return 1;
    }

    // Check if sce_sys directory was created
    fs::path sce_sys_path = output_dir / "sce_sys";
    if (!fs::exists(sce_sys_path)) {
        std::cerr << "Warning: sce_sys directory was not created!\n";
    } else {
        std::cout << "✓ sce_sys directory created\n";
    }

    // Extract all files from PFS
    int num_files = pkg.GetNumberOfFiles();
    bool eboot_found = false;
    fs::path eboot_path;

    if (num_files > 0) {
        std::cout << "\nExtracting " << num_files << " files from PFS image...\n";
        
        for (int i = 0; i < num_files; i++) {
            try {
                pkg.ExtractFiles(i);
                
                if ((i + 1) % 100 == 0 || i == num_files - 1) {
                    std::cout << "  Progress: " << (i + 1) << "/" << num_files << " files\n";
                }
            } catch (const std::exception& e) {
                std::cerr << "Warning: Failed to extract file " << i << ": " << e.what() << std::endl;
            }
        }
        
        std::cout << "\nSearching for eboot.bin...\n";
        
        // Search for eboot.bin
        if (FindEboot(output_dir, eboot_path)) {
            eboot_found = true;
            std::cout << "✓ eboot.bin found at: " << eboot_path << "\n";
        } else if (!is_patch_pkg) {
            std::cerr << "Warning: eboot.bin not found in output directory.\n";
            std::cerr << "         The PKG may be incomplete or corrupted.\n";
        } else {
            std::cout << "Info: Patch PKG does not contain eboot.bin (expected).\n";
        }
    } else {
        std::cout << "Warning: No files found in PFS image.\n";
        std::cout << "         Only metadata files were extracted.\n";
    }

    std::cout << "\n================\n";
    std::cout << "Unpacking completed!\n";
    std::cout << "Output directory: " << output_dir << "\n";
    
    // Verify required files exist
    bool has_sce_sys = fs::exists(sce_sys_path) && fs::is_directory(sce_sys_path);
    bool has_eboot = eboot_found;
    const bool require_eboot = !is_patch_pkg;
    
    std::cout << "\nVerification:\n";
    if (has_sce_sys && (has_eboot || !require_eboot)) {
        std::cout << "✓ All required files unpacked successfully!\n";
        std::cout << "  - sce_sys directory: ✓\n";
        std::cout << "  - eboot.bin: " << (has_eboot ? "✓" : "not applicable for patch") << "\n";
        if (!is_patch_pkg) {
            std::cout << "\nThe unpacked game is ready to run with shadPS4 emulator.\n";
        } else {
            std::cout << "\nPatch content unpacked into base directory: " << output_dir
                      << ".\n";
        }
        return 0;
    } else {
        std::cerr << "\n⚠ Warning: Some required files may be missing:\n";
        if (!has_sce_sys) {
            std::cerr << "  - sce_sys directory: ✗\n";
        } else {
            std::cout << "  - sce_sys directory: ✓\n";
        }
        if (require_eboot && !has_eboot) {
            std::cerr << "  - eboot.bin: ✗\n";
        } else if (!require_eboot) {
            std::cout << "  - eboot.bin: not required for patch PKG\n";
        } else {
            std::cout << "  - eboot.bin: ✓\n";
        }
        return 1;
    }
}

