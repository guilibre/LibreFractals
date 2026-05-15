export async function compress(str: string): Promise<string> {
  const cs = new CompressionStream('deflate-raw');
  const writer = cs.writable.getWriter();
  const bufPromise = new Response(cs.readable).arrayBuffer();
  await writer.write(new TextEncoder().encode(str));
  await writer.close();
  const buf = await bufPromise;
  const bytes = new Uint8Array(buf);
  let bin = '';
  for (let i = 0; i < bytes.length; i++) bin += String.fromCharCode(bytes[i]);
  return btoa(bin).replace(/\+/g, '-').replace(/\//g, '_').replace(/=/g, '');
}

export async function decompress(b64url: string): Promise<string> {
  const b64 = b64url.replace(/-/g, '+').replace(/_/g, '/');
  const bytes = Uint8Array.from(atob(b64), (c) => c.charCodeAt(0));
  const ds = new DecompressionStream('deflate-raw');
  const writer = ds.writable.getWriter();
  const bufPromise = new Response(ds.readable).arrayBuffer();
  await writer.write(bytes);
  await writer.close();
  return new TextDecoder().decode(await bufPromise);
}
