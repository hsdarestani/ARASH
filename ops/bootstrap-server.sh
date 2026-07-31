#!/usr/bin/env bash
set -euo pipefail

DOMAIN="arash.smarbiz.sbs"
WEBROOT="/var/www/arash"
NGINX_CONF="/etc/nginx/sites-available/arash"

export DEBIAN_FRONTEND=noninteractive
apt-get update
apt-get install -y nginx certbot python3-certbot-nginx curl git jq

mkdir -p "$WEBROOT"
cat > "$WEBROOT/index.html" <<'HTML'
<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8" />
  <meta name="viewport" content="width=device-width,initial-scale=1" />
  <title>ARASH</title>
  <style>
    html,body{height:100%;margin:0;background:#0b0a08;color:#e8d7a6;font-family:system-ui,-apple-system,Segoe UI,sans-serif}
    body{display:grid;place-items:center;background:radial-gradient(circle at 50% 30%,#342315 0,#15100c 38%,#090806 76%)}
    main{text-align:center;padding:32px;max-width:760px}
    h1{font-size:clamp(56px,12vw,144px);letter-spacing:.22em;margin:0 0 8px;text-indent:.22em}
    p{color:#b8aa86;font-size:18px;line-height:1.6}
    small{color:#776d58}
  </style>
</head>
<body>
  <main>
    <h1>ARASH</h1>
    <p>Persian mythic action roguelite — vertical slice in development.</p>
    <small>One arrow. An army.</small>
  </main>
</body>
</html>
HTML

cat > "$NGINX_CONF" <<NGINX
server {
    listen 80;
    listen [::]:80;
    server_name ${DOMAIN};

    root ${WEBROOT};
    index index.html;

    location / {
        try_files \$uri \$uri/ /index.html;
    }

    location = /health {
        default_type application/json;
        return 200 '{"status":"ok","service":"arash"}';
    }
}
NGINX

ln -sfn "$NGINX_CONF" /etc/nginx/sites-enabled/arash
rm -f /etc/nginx/sites-enabled/default
nginx -t
systemctl enable --now nginx
systemctl reload nginx

# Obtain TLS only when DNS already resolves to this host. Failure does not break bootstrap.
if getent ahostsv4 "$DOMAIN" >/dev/null 2>&1; then
  certbot --nginx -d "$DOMAIN" --non-interactive --agree-tos --register-unsafely-without-email --redirect || true
fi

echo "ARASH server bootstrap complete"
curl -fsS http://127.0.0.1/health || true
