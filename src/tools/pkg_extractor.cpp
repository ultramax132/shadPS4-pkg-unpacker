// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <iostream>
#include <filesystem>
#include <string>
#include <vector>
#include <iomanip>
#include "core/file_format/pkg.h"

namespace fs = std::filesystem;

void PrintUsage(const char* program_name) {
    std::cout << "PS4 PKG Extractor for shadPS4\n";
    std::cout << "Usage: " << program_name << " <pkg_file> [output_directory]\n\n";
    std::cout << "Arguments:\n";
    std::cout << "  pkg_file          Path to the .pkg file to extract\n";
    std::cout << "  output_directory   (Optional) Output directory for extracted files\n";
    std::cout << "                     If not specified, defaults to <pkg_file>.extracted\n\n";
    std::cout << "Example:\n";
    std::cout << "  " << program_name << " game.pkg\n";
    std::cout << "  " << program_name << " game.pkg /path/to/output\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2 || argc > 3) {
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

    // Determine output directory
    fs::path output_dir;
    if (argc == 3) {
        output_dir = argv[2];
    } else {
        output_dir = pkg_path;
        output_dir += ".extracted";
    }

    // Create output directory
    try {
        fs::create_directories(output_dir);
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Error: Failed to create output directory: " << e.what() << std::endl;
        return 1;
    }

    std::cout << "PS4 PKG Extractor\n";
    std::cout << "==================\n";
    std::cout << "Input file:  " << pkg_path << "\n";
    std::cout << "Output dir:  " << output_dir << "\n\n";

    // Open and extract PKG
    PKG pkg;
    std::string failreason;

    std::cout << "Opening PKG file...\n";
    if (!pkg.Open(pkg_path, failreason)) {
        std::cerr << "Error: Failed to open PKG file: " << failreason << std::endl;
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
    if (num_files > 0) {
        std::cout << "\nExtracting " << num_files << " files from PFS image...\n";
        
        bool eboot_found = false;
        for (int i = 0; i < num_files; i++) {
            try {
                pkg.ExtractFiles(i);
                
                // Check if this is eboot.bin
                // We need to check the extracted paths, but since ExtractFiles doesn't return the path,
                // we'll check if eboot.bin exists after extraction
                
                if ((i + 1) % 10 == 0 || i == num_files - 1) {
                    std::cout << "  Progress: " << (i + 1) << "/" << num_files << " files\n";
                }
            } catch (const std::exception& e) {
                std::cerr << "Warning: Failed to extract file " << i << ": " << e.what() << std::endl;
            }
        }
        
        // Check for eboot.bin in common locations
        std::vector<fs::path> eboot_paths = {
            output_dir / "eboot.bin",
            output_dir / "app0" / "eboot.bin"
        };
        
        for (const auto& eboot_path : eboot_paths) {
            if (fs::exists(eboot_path)) {
                eboot_found = true;
                std::cout << "✓ eboot.bin found at: " << eboot_path << "\n";
                break;
            }
        }
        
        if (!eboot_found) {
            std::cout << "Warning: eboot.bin not found in expected locations.\n";
            std::cout << "         Searching for eboot.bin in output directory...\n";
            
            // Recursive search for eboot.bin
            bool found = false;
            for (const auto& entry : fs::recursive_directory_iterator(output_dir)) {
                if (entry.is_regular_file() && entry.path().filename() == "eboot.bin") {
                    std::cout << "✓ eboot.bin found at: " << entry.path() << "\n";
                    found = true;
                    break;
                }
            }
            
            if (!found) {
                std::cerr << "Error: eboot.bin was not extracted!\n";
                std::cerr << "       The PKG may be incomplete or corrupted.\n";
            }
        }
    } else {
        std::cout << "Warning: No files found in PFS image.\n";
        std::cout << "         Only metadata files were extracted.\n";
    }

    std::cout << "\n==================\n";
    std::cout << "Extraction completed!\n";
    std::cout << "Output directory: " << output_dir << "\n";
    
    // Verify required files exist
    bool has_sce_sys = fs::exists(sce_sys_path);
    bool has_eboot = false;
    
    for (const auto& eboot_path : eboot_paths) {
        if (fs::exists(eboot_path)) {
            has_eboot = true;
            break;
        }
    }
    
    if (!has_eboot) {
        for (const auto& entry : fs::recursive_directory_iterator(output_dir)) {
            if (entry.is_regular_file() && entry.path().filename() == "eboot.bin") {
                has_eboot = true;
                break;
            }
        }
    }
    
    if (has_sce_sys && has_eboot) {
        std::cout << "\n✓ All required files extracted successfully!\n";
        std::cout << "  - sce_sys directory: ✓\n";
        std::cout << "  - eboot.bin: ✓\n";
        std::cout << "\nThe extracted game is ready to run with shadPS4 emulator.\n";
        return 0;
    } else {
        std::cerr << "\n⚠ Warning: Some required files may be missing:\n";
        if (!has_sce_sys) {
            std::cerr << "  - sce_sys directory: ✗\n";
        } else {
            std::cout << "  - sce_sys directory: ✓\n";
        }
        if (!has_eboot) {
            std::cerr << "  - eboot.bin: ✗\n";
        } else {
            std::cout << "  - eboot.bin: ✓\n";
        }
        return 1;
    }
}

