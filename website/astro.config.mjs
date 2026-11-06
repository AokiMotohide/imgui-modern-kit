import { defineConfig } from 'astro/config';
import starlight from '@astrojs/starlight';

export default defineConfig({
	site: 'https://aokimotohide.github.io',
	base: '/imgui-modern-kit/',
	integrations: [
		starlight({
			title: 'ImKit',
			description: 'Host-owned UI modules for creative tools built with Dear ImGui.',
			defaultLocale: 'root',
			locales: {
				root: { label: '日本語', lang: 'ja' },
				en: { label: 'English', lang: 'en' },
			},
			logo: {
				src: './src/assets/imkit-mark.svg',
				alt: 'ImKit',
			},
			social: [
				{
					icon: 'github',
					label: 'GitHub',
					href: 'https://github.com/AokiMotohide/imgui-modern-kit',
				},
			],
			customCss: ['./src/styles/custom.css'],
			components: {
				Hero: './src/components/HomeHero.astro',
			},
			sidebar: [
				{
					label: 'はじめる',
					translations: { en: 'Start here' },
					items: [
						{ slug: 'getting-started' },
						{ slug: 'guides/how-it-works' },
						{ slug: 'guides/user-guide' },
						{ slug: 'guides/gallery' },
						{ slug: 'guides/examples' },
					],
				},
				{
					label: 'チュートリアル',
					translations: { en: 'Tutorials' },
					items: [
						{ slug: 'tutorials/first-app' },
						{ slug: 'tutorials/settings-screen' },
						{ slug: 'tutorials/node-editor' },
						{ slug: 'tutorials/timeline' },
						{ slug: 'tutorials/custom-component' },
					],
				},
				{
					label: '機能',
					translations: { en: 'Features' },
					items: [
						{ slug: 'features' },
						{
							label: '部品と見た目',
							translations: { en: 'Controls and look' },
							items: [
								{ slug: 'features/components' },
								{ slug: 'features/themes' },
								{ slug: 'features/make-your-own-look' },
								{ slug: 'features/design-system' },
								{ slug: 'features/icons' },
								{ slug: 'features/toasts' },
							],
						},
						{
							label: 'データと機器の操作',
							translations: { en: 'Data and devices' },
							items: [{ slug: 'features/charts' }, { slug: 'features/ptz' }, { slug: 'features/show' }],
						},
						{
							label: '作業画面と編集',
							translations: { en: 'Workflow and editors' },
							items: [
								{ slug: 'features/workflow' },
								{ slug: 'features/shell' },
								{ slug: 'features/node-editor' },
								{ slug: 'features/editor-suite' },
								{ slug: 'features/timeline' },
								{ slug: 'features/window-frame' },
							],
						},
					],
				},
				{
					label: '設計と環境',
					translations: { en: 'Design and platform' },
					items: [
						{ slug: 'guide/architecture' },
						{ slug: 'platform/dependencies' },
						{ slug: 'platform/faq' },
						{ slug: 'platform/troubleshooting' },
					],
				},
				{
					label: '移行',
					translations: { en: 'Migration' },
					items: [{ slug: 'platform/migration-v4' }, { slug: 'platform/migration' }],
				},
				{
					label: 'API',
					items: [
						{ slug: 'api' },
						{ label: 'BeginEditor', translations: { en: 'BeginEditor' }, slug: 'api/node-editor/begin-editor' },
						{ slug: 'api/node-editor' },
						{ slug: 'api/native' },
						{ slug: 'api/v4-1' },
						{ slug: 'api/v4-0' },
						{ slug: 'api/v3-3' },
						{ slug: 'api/v3-2' },
						{ label: 'Design-system API', translations: { en: 'Design-system API' }, slug: 'api/design-system' },
						{ slug: 'api/editor-suite' },
						{ slug: 'api/window-frame' },
						{ slug: 'api/widgets' },
					],
				},
			],
		}),
	],
});
