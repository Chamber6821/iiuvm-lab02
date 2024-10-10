BUILD_DIR = build
COMPILE_COMMANDS = $(BUILD_DIR)/compile_commands.json
CPP_FLAGS = --std=c++20 -static
GPP = x86_64-w64-mingw32-g++ $(CPP_FLAGS)

.PHONE: app
app: $(BUILD_DIR)/main.exe $(BUILD_DIR)/pci.ids

.PHONE: bear
bear:
	make clean
	mkdir -p $(dir $(COMPILE_COMMANDS))
	bear --output $(COMPILE_COMMANDS) -- make app

$(BUILD_DIR)/main.exe: src/main.cpp $(BUILD_DIR)
	$(GPP) -o $@ $< 

$(BUILD_DIR)/pci.ids: ./other/pci.ids $(BUILD_DIR)
	cp $< $@

$(BUILD_DIR): %:
	mkdir $@

clean:
	rm -rf $(BUILD_DIR)

