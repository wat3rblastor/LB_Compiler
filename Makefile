all: langs
	
langs: L1_lang L2_lang L3_lang IR_lang LA_lang LB_lang

L1_lang:
	cd src/L1 ; make 

L2_lang:
	cd src/L2 ; make

L3_lang:
	cd src/L3 ; make

IR_lang:
	cd src/IR ; make

LA_lang:
	cd src/LA ; make

LB_lang:
	cd src/LB ; make

test:
	./scripts/tests.sh

benchmark:
	cd src/LB ; make benchmark

exec_l1:
	@[ -n "$(PROGRAM)" ] || (echo "Usage: make exec_l1 PROGRAM=programs/l1/scratch/file.L1 [OUTPUT=build/programs/l1/file.out]" && exit 1)
	./scripts/program.sh build --stage L1 --program "$(PROGRAM)" $(if $(OUTPUT),--output "$(OUTPUT)")

exec_l2:
	@[ -n "$(PROGRAM)" ] || (echo "Usage: make exec_l2 PROGRAM=programs/l2/scratch/file.L2 [OUTPUT=build/programs/l2/file.out]" && exit 1)
	./scripts/program.sh build --stage L2 --program "$(PROGRAM)" $(if $(OUTPUT),--output "$(OUTPUT)")

exec_l3:
	@[ -n "$(PROGRAM)" ] || (echo "Usage: make exec_l3 PROGRAM=programs/l3/scratch/file.L3 [OUTPUT=build/programs/l3/file.out]" && exit 1)
	./scripts/program.sh build --stage L3 --program "$(PROGRAM)" $(if $(OUTPUT),--output "$(OUTPUT)")

exec_ir:
	@[ -n "$(PROGRAM)" ] || (echo "Usage: make exec_ir PROGRAM=programs/ir/scratch/file.IR [OUTPUT=build/programs/ir/file.out]" && exit 1)
	./scripts/program.sh build --stage IR --program "$(PROGRAM)" $(if $(OUTPUT),--output "$(OUTPUT)")

exec_la:
	@[ -n "$(PROGRAM)" ] || (echo "Usage: make exec_la PROGRAM=programs/la/scratch/file.a [OUTPUT=build/programs/la/file.out]" && exit 1)
	./scripts/program.sh build --stage LA --program "$(PROGRAM)" $(if $(OUTPUT),--output "$(OUTPUT)")

exec_lb:
	@[ -n "$(PROGRAM)" ] || (echo "Usage: make exec_lb PROGRAM=programs/lb/scratch/file.b [OUTPUT=build/programs/lb/file.out]" && exit 1)
	./scripts/program.sh build --stage LB --program "$(PROGRAM)" $(if $(OUTPUT),--output "$(OUTPUT)")

run_l1:
	@[ -n "$(PROGRAM)" ] || (echo "Usage: make run_l1 PROGRAM=programs/l1/scratch/file.L1 [INPUT=path/to/input] [OUTPUT=build/programs/l1/file.out]" && exit 1)
	./scripts/program.sh run --stage L1 --program "$(PROGRAM)" $(if $(INPUT),--input "$(INPUT)") $(if $(OUTPUT),--output "$(OUTPUT)")

run_l2:
	@[ -n "$(PROGRAM)" ] || (echo "Usage: make run_l2 PROGRAM=programs/l2/scratch/file.L2 [INPUT=path/to/input] [OUTPUT=build/programs/l2/file.out]" && exit 1)
	./scripts/program.sh run --stage L2 --program "$(PROGRAM)" $(if $(INPUT),--input "$(INPUT)") $(if $(OUTPUT),--output "$(OUTPUT)")

run_l3:
	@[ -n "$(PROGRAM)" ] || (echo "Usage: make run_l3 PROGRAM=programs/l3/scratch/file.L3 [INPUT=path/to/input] [OUTPUT=build/programs/l3/file.out]" && exit 1)
	./scripts/program.sh run --stage L3 --program "$(PROGRAM)" $(if $(INPUT),--input "$(INPUT)") $(if $(OUTPUT),--output "$(OUTPUT)")

run_ir:
	@[ -n "$(PROGRAM)" ] || (echo "Usage: make run_ir PROGRAM=programs/ir/scratch/file.IR [INPUT=path/to/input] [OUTPUT=build/programs/ir/file.out]" && exit 1)
	./scripts/program.sh run --stage IR --program "$(PROGRAM)" $(if $(INPUT),--input "$(INPUT)") $(if $(OUTPUT),--output "$(OUTPUT)")

run_la:
	@[ -n "$(PROGRAM)" ] || (echo "Usage: make run_la PROGRAM=programs/la/scratch/file.a [INPUT=path/to/input] [OUTPUT=build/programs/la/file.out]" && exit 1)
	./scripts/program.sh run --stage LA --program "$(PROGRAM)" $(if $(INPUT),--input "$(INPUT)") $(if $(OUTPUT),--output "$(OUTPUT)")

run_lb:
	@[ -n "$(PROGRAM)" ] || (echo "Usage: make run_lb PROGRAM=programs/lb/scratch/file.b [INPUT=path/to/input] [OUTPUT=build/programs/lb/file.out]" && exit 1)
	./scripts/program.sh run --stage LB --program "$(PROGRAM)" $(if $(INPUT),--input "$(INPUT)") $(if $(OUTPUT),--output "$(OUTPUT)")

clean:
	rm -f *.bz2 ; 
	cd src/L1 ; make clean ; 
	cd src/L2 ; make clean ; 
	cd src/L3 ; make clean ; 
	cd src/IR ; make clean ; 
	cd src/LA ; make clean ; 
	cd src/LB ; make clean ; 

.PHONY: langs L1_lang L2_lang L3_lang IR_lang LA_lang LB_lang test benchmark \
	exec_l1 exec_l2 exec_l3 exec_ir exec_la exec_lb \
	run_l1 run_l2 run_l3 run_ir run_la run_lb clean
