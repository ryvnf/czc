ELF=cat sort life
CZC=../czc
C_FILES=$(patsubst %,%.c,$(ELF))

.PHONY: all c
all: $(ELF)
c: $(C_FILES)

%: %.z
	$(CZC) -o $@ $^

%.c: %.z
	$(CZC) -C -o $@ $^

.PHONY: clean
clean:
	$(RM) $(ELF) $(C_FILES)
