BUILD_DIR=build
WORK_DIR=workdir

run: $(BUILD_DIR)/main.exe $(WORK_DIR)
	cd $(WORK_DIR) && ..\\$(BUILD_DIR)\\main.exe

run-%: $(BUILD_DIR)/main.exe $(WORK_DIR)
	cd $(WORK_DIR) && ..\\$(BUILD_DIR)\\main.exe $*

$(BUILD_DIR)/main.exe: src/main.cpp $(BUILD_DIR)
	g++ --std=c++14 -o $@ $<

$(BUILD_DIR) $(WORK_DIR): %:
	mkdir $@

