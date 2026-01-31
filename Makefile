.PHONY: clone clean clean-all

LLVM_VERSION := 22.1.0-rc2
BUILD_MODE := Release

SOURCE_DIR := ./llvm
BUILD_DIR  := $(SOURCE_DIR)/build
CHK_DIR    := ./chk
INSTALL_DIR_NAME := llvm-$(LLVM_VERSION)-$(BUILD_MODE)
INSTALL_DIR := ./$(INSTALL_DIR_NAME)
ARCHIVE_NAME := "$(INSTALL_DIR_NAME).7z"

$(CHK_DIR)/7zip_chk: $(CHK_DIR)/llvm_chk
	$(MAKE) dirs
	7z a -mx9 $(ARCHIVE_NAME) $(INSTALL_DIR)/*
	touch $(CHK_DIR)/7zip_chk

$(CHK_DIR)/llvm_chk: $(CHK_DIR)/conf_chk
	$(MAKE) dirs
	cmake --build $(BUILD_DIR) --target install --config Release -- -j 12
	touch $(CHK_DIR)/llvm_chk

$(CHK_DIR)/conf_chk: $(CHK_DIR)/repo_chk
	$(MAKE) dirs
	cmake -S "$(SOURCE_DIR)/llvm" \
	      -B $(BUILD_DIR) \
	      -G Ninja \
	      -DLLVM_ENABLE_PROJECTS="mlir;lldb;clang;lld" \
	      -DLLVM_USE_LINKER=lld \
	      -DLLVM_BUILD_LLVM_DYLIB=ON \
	      -DLLVM_LINK_LLVM_DYLIB=ON \
	      -DCMAKE_BUILD_TYPE=$(BUILD_MODE) \
	      -DCMAKE_INSTALL_PREFIX=$(INSTALL_DIR) \
	      -DLLVM_CCACHE_BUILD=ON \
	      -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded \
	      -DCMAKE_CXX_FLAGS="-D_SILENCE_NONFLOATING_COMPLEX_DEPRECATION_WARNING"
	touch $(CHK_DIR)/conf_chk

$(CHK_DIR)/repo_chk:
	git clone --single-branch --branch "llvmorg-$(LLVM_VERSION)" --depth 1 "https://github.com/llvm/llvm-project.git" $(SOURCE_DIR)
	$(MAKE) dirs
	touch $(CHK_DIR)/repo_chk

clone: $(CHK_DIR)/repo_chk

dirs:
	mkdir -p $(CHK_DIR)
	mkdir -p $(BUILD_DIR)
	mkdir -p $(INSTALL_DIR)

clean:
	rm -f $(CHK_DIR)/conf_chk
	rm -f $(CHK_DIR)/llvm_chk
	rm -f $(CHK_DIR)/7zip_chk
	rm -rf $(BUILD_DIR)

clean-all: clean
	rm -rf $(CHK_DIR)
	rm -rf $(SOURCE_DIR)
	rm -rf $(INSTALL_DIR)
	rm -rf $(ARCHIVE_NAME)
