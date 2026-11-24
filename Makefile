.PHONY: all app client test clean docker-build lint format

all: app client

app:
	$(MAKE) -C app

client:
	cd client && npm ci && npm run build

test: test-unit test-functional test-client

test-unit:
	$(MAKE) -C tests/unit run

test-functional:
	cd tests/functional && pip install -r requirements.txt -q && \
	pytest -v --tb=short

test-client:
	cd client && npm test

clean:
	$(MAKE) -C app clean
	cd client && rm -rf dist node_modules
	rm -rf tests/unit/*.o tests/unit/test_runner

docker-build:
	docker run --rm -v "$(CURDIR)/app:/app" \
		ghcr.io/ledgerhq/ledger-app-builder/ledger-app-builder:latest \
		bash -c "cd /app && make"

docker-build-stax:
	docker run --rm -v "$(CURDIR)/app:/app" \
		ghcr.io/ledgerhq/ledger-app-builder/ledger-app-builder:latest \
		bash -c "cd /app && BOLOS_SDK=\$$STAX_SDK make"

docker-build-all:
	@for sdk in NANOS_SDK NANOX_SDK NANOSP_SDK STAX_SDK FLEX_SDK; do \
		echo "=== Building for $$sdk ==="; \
		docker run --rm -v "$(CURDIR)/app:/app" \
			ghcr.io/ledgerhq/ledger-app-builder/ledger-app-builder:latest \
			bash -c "cd /app && BOLOS_SDK=\$$$$sdk make"; \
	done

lint: lint-c lint-ts

lint-c:
	find app/src -name '*.c' -o -name '*.h' | xargs clang-format --dry-run --Werror

lint-ts:
	cd client && npx eslint src/

format:
	find app/src -name '*.c' -o -name '*.h' | xargs clang-format -i
	cd client && npx eslint src/ --fix
