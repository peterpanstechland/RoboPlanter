FROM python:3.12-slim
ENV PYTHONDONTWRITEBYTECODE=1 PYTHONUNBUFFERED=1 PIP_DISABLE_PIP_VERSION_CHECK=1
WORKDIR /app
COPY integrations/mcp/requirements.txt integrations/mcp/requirements.txt
COPY integrations/cloud/requirements.txt integrations/cloud/requirements.txt
RUN pip install --no-cache-dir -r integrations/cloud/requirements.txt
# Explicit file copies prevent local .env files and tools from entering layers.
COPY integrations/mcp/server.py integrations/mcp/bridge.py integrations/mcp/
COPY integrations/cloud/server.py integrations/cloud/mqtt_bridge.py integrations/cloud/protocol.py integrations/cloud/
USER 65532:65532
EXPOSE 8765
CMD ["python", "-m", "integrations.cloud.server"]
