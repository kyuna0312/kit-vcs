# Phase 4: Web UI (Vue SPA) — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build a full GitHub-lite Vue 3 SPA that connects to `kit-server` REST API — repo browser, commit log, diff viewer, branches, PRs, issues.

**Architecture:** Vite + Vue 3 + TypeScript. Pinia for state, Vue Router for navigation. Typed API layer wrapping fetch calls. Custom components (no UI library). Built output served by `kit-server` as static files from `web/dist/`.

**Tech Stack:** Node.js 20+, Vue 3.4+, Vite 5+, TypeScript 5+, Pinia 2+, Vue Router 4+, highlight.js 11+

**Prerequisite:** Phase 3 complete — `kit-server` REST API running on `:8080`.

---

## File Map

### Created
```
web/package.json
web/vite.config.ts
web/tsconfig.json
web/index.html
web/src/main.ts
web/src/App.vue
web/src/router/index.ts
web/src/stores/repos.ts
web/src/stores/commits.ts
web/src/api/client.ts
web/src/api/repos.ts
web/src/api/commits.ts
web/src/api/branches.ts
web/src/api/pulls.ts
web/src/api/issues.ts
web/src/components/common/Button.vue
web/src/components/common/Badge.vue
web/src/components/common/Modal.vue
web/src/components/domain/CommitGraph.vue
web/src/components/domain/DiffViewer.vue
web/src/components/domain/FileBrowser.vue
web/src/views/Home.vue
web/src/views/Repo.vue
web/src/views/Commits.vue
web/src/views/Diff.vue
web/src/views/Branches.vue
web/src/views/PullRequest.vue
web/src/views/Issues.vue
web/src/views/Settings.vue
```

---

### Task 1: Vite project setup

**Files:**
- Create: `web/package.json`
- Create: `web/vite.config.ts`
- Create: `web/tsconfig.json`
- Create: `web/index.html`
- Create: `web/src/main.ts`

- [ ] **Step 1: Initialize project**

```bash
cd /home/kyuna/Desktop/kit-vcs
npm create vite@latest web -- --template vue-ts
cd web
npm install
npm install pinia vue-router@4 highlight.js
```

- [ ] **Step 2: Configure vite.config.ts for API proxy**

Replace `web/vite.config.ts`:
```typescript
import { defineConfig } from 'vite'
import vue from '@vitejs/plugin-vue'

export default defineConfig({
  plugins: [vue()],
  server: {
    proxy: {
      '/repos': 'http://localhost:8080',
      '/health': 'http://localhost:8080',
    }
  }
})
```

- [ ] **Step 3: Set up main.ts**

```typescript
import { createApp } from 'vue'
import { createPinia } from 'pinia'
import App from './App.vue'
import { router } from './router'

const app = createApp(App)
app.use(createPinia())
app.use(router)
app.mount('#app')
```

- [ ] **Step 4: Verify dev server starts**

```bash
npm run dev
```

Expected: `http://localhost:5173` returns Vue app in browser.

- [ ] **Step 5: Commit**

```bash
git add web/
git commit -m "feat(web): init Vite + Vue 3 + TypeScript project"
```

---

### Task 2: Router + API client

**Files:**
- Create: `web/src/router/index.ts`
- Create: `web/src/api/client.ts`

- [ ] **Step 1: Create router**

`web/src/router/index.ts`:
```typescript
import { createRouter, createWebHistory } from 'vue-router'
import Home from '../views/Home.vue'
import Repo from '../views/Repo.vue'
import Commits from '../views/Commits.vue'
import Branches from '../views/Branches.vue'
import Diff from '../views/Diff.vue'
import PullRequest from '../views/PullRequest.vue'
import Issues from '../views/Issues.vue'
import Settings from '../views/Settings.vue'

export const router = createRouter({
  history: createWebHistory(),
  routes: [
    { path: '/',                          component: Home },
    { path: '/:repo',                     component: Repo },
    { path: '/:repo/commits',             component: Commits },
    { path: '/:repo/branches',            component: Branches },
    { path: '/:repo/diff',                component: Diff },
    { path: '/:repo/pulls/:id?',          component: PullRequest },
    { path: '/:repo/issues/:id?',         component: Issues },
    { path: '/:repo/settings',            component: Settings },
  ]
})
```

- [ ] **Step 2: Create typed API client**

`web/src/api/client.ts`:
```typescript
const BASE = ''  // proxied via vite to :8080

async function request<T>(
  method: string,
  path: string,
  body?: unknown
): Promise<T> {
  const res = await fetch(`${BASE}${path}`, {
    method,
    headers: body ? { 'Content-Type': 'application/json' } : {},
    body: body ? JSON.stringify(body) : undefined,
  })
  if (!res.ok) {
    const err = await res.json().catch(() => ({ error: res.statusText }))
    throw new Error(err.error ?? 'Request failed')
  }
  return res.json()
}

export const api = {
  get:    <T>(path: string)              => request<T>('GET', path),
  post:   <T>(path: string, body: unknown) => request<T>('POST', path, body),
  patch:  <T>(path: string, body: unknown) => request<T>('PATCH', path, body),
  delete: <T>(path: string)              => request<T>('DELETE', path),
}
```

- [ ] **Step 3: Create typed API modules**

`web/src/api/repos.ts`:
```typescript
import { api } from './client'

export interface Repo { name: string }

export const reposApi = {
  list:   ()                  => api.get<{ repos: string[] }>('/repos'),
  get:    (name: string)      => api.get<Repo>(`/repos/${name}`),
  create: (name: string)      => api.post<Repo>('/repos', { name }),
  delete: (name: string)      => api.delete<void>(`/repos/${name}`),
}
```

`web/src/api/commits.ts`:
```typescript
import { api } from './client'

export interface Commit {
  hash: string
  author: string
  message: string
  timestamp: number
}

export const commitsApi = {
  list: (repo: string) => api.get<{ commits: Commit[] }>(`/repos/${repo}/commits`),
}
```

`web/src/api/branches.ts`:
```typescript
import { api } from './client'
export interface Branch { name: string; commit: string }
export const branchesApi = {
  list:   (repo: string)                        => api.get<{ branches: Branch[] }>(`/repos/${repo}/branches`),
  create: (repo: string, name: string, from: string) => api.post<Branch>(`/repos/${repo}/branches`, { name, from }),
}
```

`web/src/api/pulls.ts`:
```typescript
import { api } from './client'
export interface PullRequest { id: number; title: string; from: string; to: string; status: string }
export const pullsApi = {
  list:   (repo: string)                        => api.get<{ pulls: PullRequest[] }>(`/repos/${repo}/pulls`),
  create: (repo: string, p: Omit<PullRequest, 'id' | 'status'>) => api.post<PullRequest>(`/repos/${repo}/pulls`, p),
  merge:  (repo: string, id: number)            => api.patch<PullRequest>(`/repos/${repo}/pulls/${id}`, { action: 'merge' }),
  close:  (repo: string, id: number)            => api.patch<PullRequest>(`/repos/${repo}/pulls/${id}`, { action: 'close' }),
}
```

`web/src/api/issues.ts`:
```typescript
import { api } from './client'
export interface Issue { id: number; title: string; body: string; status: string }
export const issuesApi = {
  list:   (repo: string)                        => api.get<{ issues: Issue[] }>(`/repos/${repo}/issues`),
  create: (repo: string, title: string, body: string) => api.post<Issue>(`/repos/${repo}/issues`, { title, body }),
  close:  (repo: string, id: number)            => api.patch<Issue>(`/repos/${repo}/issues/${id}`, { action: 'close' }),
}
```

- [ ] **Step 4: Commit**

```bash
git add web/src/router/ web/src/api/
git commit -m "feat(web): add router and typed API layer"
```

---

### Task 3: Pinia stores

**Files:**
- Create: `web/src/stores/repos.ts`
- Create: `web/src/stores/commits.ts`

- [ ] **Step 1: Create repos store**

`web/src/stores/repos.ts`:
```typescript
import { defineStore } from 'pinia'
import { ref } from 'vue'
import { reposApi, type Repo } from '../api/repos'

export const useReposStore = defineStore('repos', () => {
  const repos = ref<string[]>([])
  const current = ref<Repo | null>(null)
  const loading = ref(false)
  const error = ref<string | null>(null)

  async function fetchAll() {
    loading.value = true
    error.value = null
    try {
      const res = await reposApi.list()
      repos.value = res.repos
    } catch (e) {
      error.value = (e as Error).message
    } finally {
      loading.value = false
    }
  }

  async function fetchOne(name: string) {
    loading.value = true
    error.value = null
    try {
      current.value = await reposApi.get(name)
    } catch (e) {
      error.value = (e as Error).message
    } finally {
      loading.value = false
    }
  }

  async function createRepo(name: string) {
    const repo = await reposApi.create(name)
    repos.value.push(repo.name)
    return repo
  }

  return { repos, current, loading, error, fetchAll, fetchOne, createRepo }
})
```

`web/src/stores/commits.ts`:
```typescript
import { defineStore } from 'pinia'
import { ref } from 'vue'
import { commitsApi, type Commit } from '../api/commits'

export const useCommitsStore = defineStore('commits', () => {
  const commits = ref<Commit[]>([])
  const loading = ref(false)

  async function fetchFor(repo: string) {
    loading.value = true
    try {
      const res = await commitsApi.list(repo)
      commits.value = res.commits
    } finally {
      loading.value = false
    }
  }

  return { commits, loading, fetchFor }
})
```

- [ ] **Step 2: Commit**

```bash
git add web/src/stores/
git commit -m "feat(web): add Pinia stores for repos and commits"
```

---

### Task 4: Core views

**Files:**
- Create: `web/src/views/Home.vue`
- Create: `web/src/views/Repo.vue`
- Create: `web/src/views/Commits.vue`

- [ ] **Step 1: Home view — repo list**

`web/src/views/Home.vue`:
```vue
<template>
  <div class="home">
    <header>
      <h1>kit-vcs</h1>
      <button @click="showCreate = true">New Repository</button>
    </header>

    <div v-if="store.loading">Loading...</div>
    <div v-else-if="store.error" class="error">{{ store.error }}</div>
    <ul v-else class="repo-list">
      <li v-for="name in store.repos" :key="name">
        <router-link :to="`/${name}`">{{ name }}</router-link>
      </li>
      <li v-if="store.repos.length === 0" class="empty">No repositories yet.</li>
    </ul>

    <Modal v-if="showCreate" title="New Repository" @close="showCreate = false">
      <input v-model="newName" placeholder="repository name" />
      <button @click="createRepo">Create</button>
    </Modal>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted } from 'vue'
import { useReposStore } from '../stores/repos'
import Modal from '../components/common/Modal.vue'

const store = useReposStore()
const showCreate = ref(false)
const newName = ref('')

onMounted(() => store.fetchAll())

async function createRepo() {
  if (!newName.value.trim()) return
  await store.createRepo(newName.value.trim())
  showCreate.value = false
  newName.value = ''
}
</script>
```

- [ ] **Step 2: Commits view**

`web/src/views/Commits.vue`:
```vue
<template>
  <div class="commits">
    <h2>Commits — {{ $route.params.repo }}</h2>
    <div v-if="store.loading">Loading...</div>
    <table v-else>
      <thead>
        <tr><th>Hash</th><th>Author</th><th>Message</th><th>Date</th></tr>
      </thead>
      <tbody>
        <tr v-for="c in store.commits" :key="c.hash">
          <td><code>{{ c.hash.slice(0, 7) }}</code></td>
          <td>{{ c.author }}</td>
          <td>{{ c.message }}</td>
          <td>{{ new Date(c.timestamp * 1000).toLocaleDateString() }}</td>
        </tr>
      </tbody>
    </table>
  </div>
</template>

<script setup lang="ts">
import { onMounted } from 'vue'
import { useRoute } from 'vue-router'
import { useCommitsStore } from '../stores/commits'

const route = useRoute()
const store = useCommitsStore()
onMounted(() => store.fetchFor(route.params.repo as string))
</script>
```

- [ ] **Step 3: Repo view (overview)**

`web/src/views/Repo.vue`:
```vue
<template>
  <div class="repo">
    <nav>
      <router-link :to="`/${name}/commits`">Commits</router-link>
      <router-link :to="`/${name}/branches`">Branches</router-link>
      <router-link :to="`/${name}/pulls`">Pull Requests</router-link>
      <router-link :to="`/${name}/issues`">Issues</router-link>
      <router-link :to="`/${name}/settings`">Settings</router-link>
    </nav>
    <h2>{{ name }}</h2>
    <p>Clone via TCP: <code>kit clone {{ name }} --host localhost:9418</code></p>
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { useRoute } from 'vue-router'
const route = useRoute()
const name = computed(() => route.params.repo as string)
</script>
```

- [ ] **Step 4: Commit**

```bash
git add web/src/views/
git commit -m "feat(web): add Home, Repo, Commits views"
```

---

### Task 5: Common components + DiffViewer

**Files:**
- Create: `web/src/components/common/Modal.vue`
- Create: `web/src/components/common/Badge.vue`
- Create: `web/src/components/domain/DiffViewer.vue`

- [ ] **Step 1: Modal component**

`web/src/components/common/Modal.vue`:
```vue
<template>
  <div class="modal-overlay" @click.self="$emit('close')">
    <div class="modal">
      <header>
        <h3>{{ title }}</h3>
        <button @click="$emit('close')">✕</button>
      </header>
      <div class="modal-body">
        <slot />
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
defineProps<{ title: string }>()
defineEmits<{ close: [] }>()
</script>
```

- [ ] **Step 2: DiffViewer with syntax highlighting**

`web/src/components/domain/DiffViewer.vue`:
```vue
<template>
  <div class="diff-viewer">
    <div v-for="(line, i) in lines" :key="i"
         :class="['diff-line', lineClass(line)]">
      <span class="line-num">{{ i + 1 }}</span>
      <pre v-html="highlight(line.slice(1))"></pre>
    </div>
  </div>
</template>

<script setup lang="ts">
import hljs from 'highlight.js'
import 'highlight.js/styles/github.css'

const props = defineProps<{ diff: string; language?: string }>()

const lines = props.diff.split('\n')

function lineClass(line: string) {
  if (line.startsWith('+')) return 'added'
  if (line.startsWith('-')) return 'removed'
  return 'context'
}

function highlight(code: string) {
  if (!props.language) return code
  try {
    return hljs.highlight(code, { language: props.language }).value
  } catch {
    return code
  }
}
</script>

<style scoped>
.diff-line { display: flex; font-family: monospace; font-size: 13px; }
.added  { background: #e6ffed; }
.removed { background: #ffeef0; }
.context { background: white; }
.line-num { width: 40px; color: #999; user-select: none; }
pre { margin: 0; flex: 1; }
</style>
```

- [ ] **Step 3: Stub remaining views**

For `Branches.vue`, `PullRequest.vue`, `Issues.vue`, `Settings.vue` — create minimal placeholders:

```vue
<!-- web/src/views/Branches.vue -->
<template><div><h2>Branches — {{ $route.params.repo }}</h2></div></template>
<script setup lang="ts">import { useRoute } from 'vue-router'; const route = useRoute()</script>
```

- [ ] **Step 4: Create App.vue with router-view**

`web/src/App.vue`:
```vue
<template>
  <div id="app">
    <router-view />
  </div>
</template>
```

- [ ] **Step 5: Build and verify**

```bash
cd web
npm run build
ls dist/
```

Expected: `dist/index.html` + assets exist.

- [ ] **Step 6: Verify kit-server serves SPA**

```bash
# Start kit-server (built with Phase 3 plan)
cd /home/kyuna/Desktop/kit-vcs/build
./server/kit-server &
# Open http://localhost:8080 — should show Vue app
curl -s http://localhost:8080/ | grep -q "kit-vcs" && echo "SPA served OK"
kill %1
```

- [ ] **Step 7: Commit**

```bash
git add web/src/
git commit -m "feat(web): add Vue SPA with Home/Commits/Repo views and DiffViewer"
```
