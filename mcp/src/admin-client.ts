/**
 * HTTP client for the FieryMUD admin API (localhost:8080)
 */

const ADMIN_URL = process.env.FIERYMUD_ADMIN_URL ?? "http://127.0.0.1:8080";
const ADMIN_TOKEN = process.env.FIERYMUD_ADMIN_TOKEN ?? "";

interface AdminResponse {
  success?: boolean;
  error?: string;
  message?: string;
  [key: string]: unknown;
}

async function adminRequest(
  method: string,
  path: string,
  body?: Record<string, unknown>
): Promise<AdminResponse> {
  const url = `${ADMIN_URL}${path}`;
  const headers: Record<string, string> = { "Content-Type": "application/json" };
  if (ADMIN_TOKEN) {
    headers["Authorization"] = `Bearer ${ADMIN_TOKEN}`;
  }
  const options: RequestInit = { method, headers };
  if (body) {
    options.body = JSON.stringify(body);
  }
  const response = await fetch(url, options);
  return (await response.json()) as AdminResponse;
}

export async function adminGet(path: string): Promise<AdminResponse> {
  return adminRequest("GET", path);
}

export async function adminPost(
  path: string,
  body: Record<string, unknown>
): Promise<AdminResponse> {
  return adminRequest("POST", path, body);
}
