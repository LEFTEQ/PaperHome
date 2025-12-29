import { defineConfig } from 'orval';

export default defineConfig({
  paperhome: {
    input: {
      target: process.env.CI
        ? './openapi.json'
        : process.env.API_DOCS_URL || 'http://localhost:3000/docs-json',
    },
    output: {
      mode: 'tags-split',
      target: './src/lib/api/generated',
      schemas: './src/lib/api/generated/models',
      client: 'react-query',
      httpClient: 'fetch',
      clean: true,
      prettier: true,
      override: {
        mutator: {
          path: './src/lib/api/mutator/client-mutator.ts',
          name: 'customFetch',
        },
        query: {
          useQuery: true,
          useMutation: true,
          options: {
            staleTime: 5 * 60 * 1000,
          },
        },
      },
    },
    hooks: {
      afterAllFilesWrite: 'prettier --write',
    },
  },
});
